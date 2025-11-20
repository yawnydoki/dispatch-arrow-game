#include "game.h"
#include <cmath>
#include <algorithm>
#include <vector>

static bool segmentsIntersect(int a, int b, int c, int d)
{
    if (a == c || a == d || b == c || b == d) return false;
    float ax = nodes[a].x, ay = nodes[a].y, bx = nodes[b].x, by = nodes[b].y;
    float cx = nodes[c].x, cy = nodes[c].y, dx = nodes[d].x, dy = nodes[d].y;

    auto orient = [](float px, float py, float qx, float qy, float rx, float ry) {
        return (qx - px) * (ry - py) - (qy - py) * (rx - px);
    };

    float o1 = orient(ax, ay, bx, by, cx, cy);
    float o2 = orient(ax, ay, bx, by, dx, dy);
    float o3 = orient(cx, cy, dx, dy, ax, ay);
    float o4 = orient(cx, cy, dx, dy, bx, by);

    return (o1 * o2 < 0.0f) && (o3 * o4 < 0.0f);
}

static bool edgeWouldCross(int a, int b)
{
    for (int i = 0; i < (int)nodes.size(); ++i)
    {
        for (int j : nodes[i].links)
        {
            if (i < j && segmentsIntersect(a, b, i, j))
                return true;
        }
    }
    return false;
}

void makeMap(int numNodes, int minLinks, int maxLinks, int numLocks)
{
    nodes.clear();
    particles.clear();
    const int N = numNodes;
    nodes.resize(N);

    std::uniform_real_distribution<float> posDistX(-MAP_WIDTH * 0.9f, MAP_WIDTH * 0.9f);
    std::uniform_real_distribution<float> posDistY(-MAP_HEIGHT * 0.9f, MAP_HEIGHT * 0.9f);
    
    std::uniform_int_distribution<int> nodeDist(0, N - 1);
    std::uniform_int_distribution<int> linkDist(minLinks, maxLinks);
    std::uniform_int_distribution<int> typeDist(0, 15);
    std::uniform_int_distribution<int> arrowDist(0, 3);
    const float MIN_DIST = 0.25f; 

    auto placeNode = [&](int idx)
    {
        int attempts = 0;
        while (attempts++ < 500)
        {
            float x = posDistX(rng);
            float y = posDistY(rng);
            bool ok = true;
            for (int j = 0; j < idx; ++j)
            {
                float dx = x - nodes[j].x;
                float dy = y - nodes[j].y;
                if (std::sqrt(dx * dx + dy * dy) < MIN_DIST)
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
            {
                nodes[idx].x = x;
                nodes[idx].y = y;
                return;
            }
        }
        nodes[idx].x = posDistX(rng);
        nodes[idx].y = posDistY(rng);
    };

    for (int i = 0; i < N; ++i)
    {
        nodes[i].links.clear();
        nodes[i].visited = false;
        nodes[i].isFinal = false;
        nodes[i].isLocked = false;
        nodes[i].isKey = false;
        nodes[i].isTargetOfKey = false;
        nodes[i].isWarpNode = false;
        nodes[i].keyNode = -1;
        nodes[i].lockNode = -1;
        nodes[i].password.clear();

        int typeRoll = typeDist(rng);
        if (typeRoll == 0) nodes[i].type = NODE_HAZARD;
        else if (typeRoll == 1) nodes[i].type = NODE_FAST;
        else if (typeRoll == 2 || typeRoll == 3) nodes[i].type = NODE_REWARD;
        else if (typeRoll == 4) nodes[i].type = NODE_WARP, nodes[i].isWarpNode = true;
        else nodes[i].type = NODE_NORMAL;

        placeNode(i);
    }

    for (int i = 0; i < N; ++i)
    {
        int desired = linkDist(rng);
        int attempts = 0;
        while ((int)nodes[i].links.size() < desired && attempts++ < 200)
        {
            int j = nodeDist(rng);
            if (j == i) continue;

            float dist = std::hypot(nodes[i].x - nodes[j].x, nodes[i].y - nodes[j].y);
            if (dist > 2.0f && attempts < 150) continue; 

            bool already = false;
            for (int k : nodes[i].links) if (k == j) { already = true; break; }
            if (already) continue;
            if ((int)nodes[j].links.size() >= maxLinks) continue;
            if (edgeWouldCross(i, j) && (attempts % 8 != 0)) continue;

            nodes[i].links.push_back(j);
            nodes[j].links.push_back(i);
        }
    }

    while (true)
    {
        std::vector<int> comp(N, -1);
        int compCount = 0;

        for (int i = 0; i < N; ++i)
        {
            if (comp[i] != -1) continue;
            std::vector<int> q;
            q.push_back(i);
            comp[i] = compCount;
            size_t head = 0;
            while(head < q.size())
            {
                int u = q[head++];
                for(int v : nodes[u].links)
                {
                    if(comp[v] == -1) {
                        comp[v] = compCount;
                        q.push_back(v);
                    }
                }
            }
            compCount++;
        }

        if (compCount == 1) break; 

        bool connected = false;
        for (int i = 0; i < N; ++i)
        {
            if (comp[i] == 0)
            {
                int bestTarget = -1;
                float minD = 10000.0f;

                for (int j = 0; j < N; ++j)
                {
                    if (comp[j] != 0)
                    {
                        float dist = std::hypot(nodes[i].x - nodes[j].x, nodes[i].y - nodes[j].y);
                        if (dist < minD && (!edgeWouldCross(i, j) || dist < 0.5f))
                        {
                            minD = dist;
                            bestTarget = j;
                        }
                    }
                }

                if (bestTarget != -1)
                {
                    nodes[i].links.push_back(bestTarget);
                    nodes[bestTarget].links.push_back(i);
                    connected = true;
                    break; 
                }
            }
        }
        if (!connected) break; 
    }

    // lock n key
    for (int i = 0; i < numLocks; ++i)
    {
        int lockNode, keyNode;
        int attempts = 0;
        bool validLocation = false;

        while (attempts < 100)
        {
            lockNode = nodeDist(rng);
            keyNode = nodeDist(rng);

            if (lockNode == keyNode ||
                nodes[lockNode].type != NODE_NORMAL || nodes[keyNode].type != NODE_NORMAL ||
                nodes[lockNode].isLocked || nodes[lockNode].isKey ||
                nodes[keyNode].isLocked || nodes[keyNode].isKey)
            {
                attempts++;
                continue;
            }

            // softlock check
            std::vector<bool> visited(N, false);
            std::vector<int> q;
            int startNode = (lockNode == 0) ? 1 : 0;
            if (N > 1) {
                q.push_back(startNode);
                visited[startNode] = true;
            }
            int visitedCount = 0;
            size_t head = 0;
            while(head < q.size())
            {
                int u = q[head++];
                visitedCount++;
                for(int v : nodes[u].links)
                    if(v != lockNode && !visited[v])
                        visited[v] = true, q.push_back(v);
            }
            if(visitedCount >= N - 1) 
            {
                validLocation = true;
                break;
            }
            attempts++;
        }

        if(!validLocation) continue;

        nodes[lockNode].isLocked = true;
        nodes[lockNode].keyNode = keyNode;
        nodes[keyNode].isKey = true;
        nodes[keyNode].lockNode = lockNode;

        for (int p = 0; p < 3; ++p)
            nodes[lockNode].password.push_back((ArrowDir)arrowDist(rng));
    }

    finalNodeSpawned = false;
    gameWon = false;
    isGameOver = false;
    finalNodeIndex = -1;
    gameMessage = "";
    gameMessageTimer = 0.0f;
    revealedPasswords.clear();
    screenShakeTimer = 0.0f;

    for (auto& n : nodes) n.visited = false;

    player.currentNode = nodeDist(rng);
    int startAttempts = 0;
    while(startAttempts++ < 100 && (nodes[player.currentNode].isKey || nodes[player.currentNode].isLocked || nodes[player.currentNode].isFinal))
        player.currentNode = nodeDist(rng);
    
    player.x = nodes[player.currentNode].x;
    player.y = nodes[player.currentNode].y;
    
    cameraX = player.x;
    cameraY = player.y;

    player.highlightedNeighbor = -1;
    player.targetNode = -1;
    player.moving = false;
    player.avatarAngle = 0.0f;
}

void spawnFinalNode()
{
    std::uniform_real_distribution<float> posDistX(-MAP_WIDTH * 0.9f, MAP_WIDTH * 0.9f);
    std::uniform_real_distribution<float> posDistY(-MAP_HEIGHT * 0.9f, MAP_HEIGHT * 0.9f);
    const float MIN_DIST = 0.16f;
    int attempts = 0;
    Node fn;
    fn.visited = false;
    fn.type = NODE_NORMAL;
    fn.isFinal = true;
    fn.isTargetOfKey = false;
    while (attempts++ < 500)
    {
        float x = posDistX(rng);
        float y = posDistY(rng);
        bool ok = true;
        for (auto& n : nodes)
        {
            float dx = x - n.x, dy = y - n.y;
            if (std::sqrt(dx * dx + dy * dy) < MIN_DIST) { ok = false; break; }
        }
        if (ok) { fn.x = x; fn.y = y; break; }
    }
    if (attempts >= 500) { fn.x = 0.7f; fn.y = 0.7f; }

    int idx = (int)nodes.size();
    nodes.push_back(fn);

    std::uniform_int_distribution<int> nodeDist(0, (int)nodes.size() - 2);
    int link1 = nodeDist(rng);
    nodes[idx].links.push_back(link1);
    nodes[link1].links.push_back(idx);

    int link2 = nodeDist(rng);
    int linkAttempts = 0;
    while (link2 == link1 && linkAttempts++ < 50) link2 = nodeDist(rng);

    if (link1 != link2)
    {
        nodes[idx].links.push_back(link2);
        nodes[link2].links.push_back(idx);
    }

    finalNodeSpawned = true;
    finalNodeIndex = idx;
    playSFX("assets/audio/Engage.wav");
    setGameMessage("The Final Node has appeared!", 3.0f);
    spawnParticles(200, fn.x, fn.y, 2.0f, 1.0f, 0.8f, 0.1f);
}