#include "game.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <map>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#ifndef TEXT
#define TEXT(s) s
#endif
#define mciSendString(...) (0)
#endif

void resetGame()
{
    currentLevel = 1;
    playerLives = STARTING_LIVES;

    makeMap(15, 1, 3, 1);
    inSequence = false;
    sequence.clear();
    player.targetNode = -1;
    finalNodeSpawned = false;
    gameWon = false;
    isGameOver = false;
    finalNodeIndex = -1;
    gameInProgress = true;
    isLevelTransitioning = false; 
    levelTransitionTimer = 0.0f;

    virus.active = false;
    // 60s base, decreases by 5s per level, min 10s
    float spawnTime = 60.0f - ((currentLevel - 1) * 5.0f);
    virusSpawnTimer = std::max(10.0f, spawnTime);
    
    virus.isAttacking = false;
    virus.attackTimer = VIRUS_ATTACK_DELAY;
    
    floatingTexts.clear();
    
    setGameMessage("New Game Started!", 2.5f);

    mciSendString(TEXT("stop music"), NULL, 0, NULL);
    mciSendString(TEXT("close music"), NULL, 0, NULL);
    playBackgroundMusic();
}

void clampHighlightedNeighbor()
{
    if (nodes.empty() || player.currentNode < 0 || player.currentNode >= (int)nodes.size())
    {
        player.highlightedNeighbor = -1;
        return;
    }
    int n = (int)nodes[player.currentNode].links.size();
    if (n == 0)
    {
        player.highlightedNeighbor = -1;
        return;
    }
    if (player.highlightedNeighbor < -1)
    {
        player.highlightedNeighbor = n - 1;
    }
    if (player.highlightedNeighbor >= n)
    {
        player.highlightedNeighbor = -1;
    }
}

void startMoveAnimation(int from, int to)
{
    if (from < 0 || from >= (int)nodes.size() || to < 0 || to >= (int)nodes.size())
        return;
    player.moveFrom = from;
    player.moveTo = to;
    player.moveStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    player.moving = true;
    playSFX(SFX_MOVE_SUCCESS); 
}

static void startRhythmSequence(int targetNodeIndex)
{
    player.targetNode = targetNodeIndex;
    Node &targetNode = nodes[targetNodeIndex];

    currentSequenceIsHazard = false;
    currentSequenceIsFast = false;
    NodeType targetType = targetNode.type;

    if (targetNode.isFinal)
        setGameMessage("Final Sequence!");
    else if (targetType == NODE_HAZARD)
    {
        currentSequenceIsHazard = true;
        currentSequenceIsFast = false;
        totalHazardSequences = 2;
        hazardSequencesRemaining = totalHazardSequences;
        setGameMessage("HAZARD! Complete 2 sequences!", 3.0f);
    }
    else if (targetType == NODE_FAST)
    {
        currentSequenceIsFast = true;
        setGameMessage("FAST! Timing is halved!", 3.0f);
    }
    else if (targetType == NODE_REWARD)
        setGameMessage("Energy Cell Targetted!");
    else if (targetNode.isKey)
        setGameMessage("Decrypter Found! Build path.");
    else if (targetType == NODE_WARP)
        setGameMessage("Warp Node... Shortcut?", 3.0f);

    std::uniform_int_distribution<int> arrowDist(0, 3);
    std::uniform_int_distribution<int> lenDist(0, 2);
    std::uniform_int_distribution<int> fakeRoll(0, 100);

    int baseLen = targetNode.isFinal ? 5 : 3;
    int levelAdd = (currentLevel / 2);
    int len = baseLen + levelAdd + lenDist(rng);
    if (currentSequenceIsHazard)
        len = 3 + (lenDist(rng) / 2);

    sequence.clear();
    for (int i = 0; i < len; ++i)
    {
        Arrow a;
        a.dir = (ArrowDir)arrowDist(rng);
        a.result = 0;
        a.displayAnim = 1.0f;
        a.isFake = false;
        if (currentSequenceIsHazard && fakeRoll(rng) < 20)
        {
            a.isFake = true;
        }
        sequence.push_back(a);
    }

    seqIdx = 0;
    inSequence = true;
    stepWaitingForInput = true;
    stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME) + 200; 
    playSFX(SFX_MENU_ENGAGE);
}

void attemptMoveToHighlightedNeighbor()
{
    if (player.moving || inSequence || displayingPassword || isPasswordInput)
        return;
    if (player.highlightedNeighbor == -1)
    {
        playSFX(SFX_TIMING_FAIL);
        return;
    }

    if (player.currentNode < 0 || player.currentNode >= (int)nodes.size())
        return;

    clampHighlightedNeighbor();
    int neighbor = nodes[player.currentNode].links[player.highlightedNeighbor];
    Node &targetNode = nodes[neighbor];

    if (targetNode.visited && !targetNode.isFinal)
    {
        startMoveAnimation(player.currentNode, neighbor);
        player.currentNode = neighbor;
        player.targetNode = -1;
        player.highlightedNeighbor = -1;
        return;
    }

    if (targetNode.isLocked)
    {
        player.targetNode = neighbor;
        inSequence = true;
        isPasswordInput = true;
        seqIdx = 0;
        sequence.clear();
        setGameMessage("Enter Password!", 2.5f);
        playSFX(SFX_MENU_ENGAGE);
        return;
    }

    startRhythmSequence(neighbor);
}

void generateNextHazardSequence()
{
    playSFX(SFX_MENU_ENGAGE);
    setGameMessage("HAZARD! Sequence " + std::to_string(totalHazardSequences - hazardSequencesRemaining + 1) + " of " + std::to_string(totalHazardSequences), 3.0f);

    std::uniform_int_distribution<int> arrowDist(0, 3);
    std::uniform_int_distribution<int> lenDist(0, 1);
    std::uniform_int_distribution<int> fakeRoll(0, 100);
    int len = 3 + lenDist(rng);

    sequence.clear();
    for (int i = 0; i < len; ++i)
    {
        Arrow a;
        a.dir = (ArrowDir)arrowDist(rng);
        a.result = 0;
        a.displayAnim = 1.0f;
        a.isFake = (fakeRoll(rng) < 20); 
        sequence.push_back(a);
    }

    seqIdx = 0;
    stepWaitingForInput = true;
    stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME) + 200;
}

static void handleSequenceFailure(bool passwordFail = false)
{
    if (!sequence.empty() && seqIdx >= 0 && seqIdx < (int)sequence.size() && !passwordFail)
    {
        sequence[seqIdx].result = -1;
        sequence[seqIdx].displayAnim = 1.2f;
    }

    playSFX(SFX_TIMING_FAIL); 
    spawnParticles(30, player.x, player.y, 1.0f, 1.0f, 0.2f, 0.2f);
    screenShakeTimer = 0.5f;
    
    spawnFloatingText(passwordFail ? "DENIED" : "MISS!", player.x, player.y + 0.1f, 1.0f, 0.2f, 0.2f, 1.5f);

    playerLives--;
    if (playerLives <= 0)
    {
        isGameOver = true;
        setGameMessage("GAME OVER. Press ESC.", 1000.0f);
    }
    else
    {
        setGameMessage(passwordFail ? ("Access Denied! Energy: " + std::to_string(playerLives)) : ("Sequence Failed! Energy: " + std::to_string(playerLives)), 3.0f);
    }

    inSequence = false;
    isPasswordInput = false;
    sequence.clear();
    seqIdx = 0;
    stepWaitingForInput = false;
    currentSequenceIsHazard = false;
    hazardSequencesRemaining = 0;
    totalHazardSequences = 0;
}

static void finishSequenceSuccess()
{
    if (currentSequenceIsHazard)
    {
        hazardSequencesRemaining--;
        if (hazardSequencesRemaining > 0)
        {
            generateNextHazardSequence();
            return;
        }
    }

    if (player.targetNode >= 0)
    {
        Node &target = nodes[player.targetNode];

        if (target.isKey)
        {
            int lockId = target.lockNode;
            sequence.clear();
            for (ArrowDir dir : nodes[lockId].password)
                sequence.emplace_back(dir, 0, 1.0f, false);

            revealedPasswords[lockId] = nodes[lockId].password;
            nodes[lockId].isTargetOfKey = true;

            displayingPassword = true;
            justRevealedPassword = true;
            inSequence = false;
            stepWaitingForInput = false;
            setGameMessage("Password Revealed! Press SPACE.", 1000.0f);
            return; 
        }
        else if (target.type == NODE_REWARD)
        {
            playerLives++;
            setGameMessage("Energy Cell Added! (Energy: " + std::to_string(playerLives) + ")", 3.0f);
            target.type = NODE_NORMAL;
            spawnFloatingText("+ENERGY", target.x, target.y + 0.1f, 0.2f, 1.0f, 1.0f, 1.2f);
        }
        else if (target.type == NODE_WARP && !target.visited)
        {
            int numWarpLinks = 1 + (rng() % 2);
            int N = (int)nodes.size();
            int createdLinks = 0;
            std::string msg = "Warp links established!";

            for (int k = 0; k < numWarpLinks; ++k)
            {
                int attempts = 0;
                while (attempts++ < 50)
                {
                    int linkTargetNode = rng() % (N - (finalNodeIndex != -1 ? 1 : 0));
                    if (linkTargetNode == player.targetNode)
                        continue;

                    bool alreadyLinked = false;
                    for (int l : target.links)
                        if (l == linkTargetNode)
                        {
                            alreadyLinked = true;
                            break;
                        }

                    if (!alreadyLinked)
                    {
                        target.links.push_back(linkTargetNode);
                        nodes[linkTargetNode].links.push_back(player.targetNode);
                        createdLinks++;
                        spawnParticles(30, target.x, target.y, 1.5f, 0.8f, 0.3f, 1.0f);
                        spawnParticles(30, nodes[linkTargetNode].x, nodes[linkTargetNode].y, 1.5f, 0.8f, 0.3f, 1.0f);
                        break;
                    }
                }
            }
            if (createdLinks == 0)
                msg = "Warp failed... no new links found.";
            else
                player.highlightedNeighbor = -1;

            setGameMessage(msg, 3.0f);
        }

        target.visited = true;
        spawnParticles(50, nodes[player.targetNode].x, nodes[player.targetNode].y, 1.0f, 0.2f, 1.0f, 0.3f);
        shockwaves.push_back({nodes[player.targetNode].x, nodes[player.targetNode].y, 0.05f, 1.0f});
        startMoveAnimation(player.currentNode, player.targetNode);
        player.currentNode = player.targetNode;
        player.targetNode = -1;
        player.highlightedNeighbor = -1;
    }

    inSequence = false;
    sequence.clear();
    seqIdx = 0;
    stepWaitingForInput = false;
    currentSequenceIsHazard = false;
    hazardSequencesRemaining = 0;
    totalHazardSequences = 0;
}

void handlePasswordInput(ArrowDir pressed)
{
    Node &lockNode = nodes[player.targetNode];
    std::vector<ArrowDir> &password = lockNode.password;

    bool correct = (seqIdx < password.size()) && (password[seqIdx] == pressed);

    if (correct)
    {
        sequence.emplace_back(pressed, 1, 1.2f);
        seqIdx++;
        playSFX(SFX_TIMING_GOOD); 

        if (seqIdx == password.size())
        {
            lockNode.isLocked = false;
            if (lockNode.keyNode != -1)
                nodes[lockNode.keyNode].isKey = false;
            lockNode.visited = true;

            inSequence = false;
            isPasswordInput = false;

            setGameMessage("Firewall Unlocked!", 3.0f);
            spawnFloatingText("UNLOCKED", lockNode.x, lockNode.y + 0.15f, 0.2f, 1.0f, 0.2f, 1.5f);
            playSFX(SFX_UNLOCK); 
            spawnParticles(100, lockNode.x, lockNode.y, 1.5f, 1.0f, 1.0f, 0.2f);

            startMoveAnimation(player.currentNode, player.targetNode);
            player.currentNode = player.targetNode;
            player.targetNode = -1;
            player.highlightedNeighbor = -1;
        }
    }
    else
    {
        handleSequenceFailure(true);
    }
}

void handleArrowPress(ArrowDir pressed)
{
    if (isPasswordInput)
    {
        handlePasswordInput(pressed);
        return;
    }
    
    if (sequence.empty() || seqIdx < 0 || seqIdx >= (int)sequence.size() || !stepWaitingForInput)
        return;

    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt = now - stepStartTimeMs;

    int currentPerfectMs = currentSequenceIsFast ? (TIMING_PERFECT_MS / 2) : TIMING_PERFECT_MS;
    int currentGoodMs = currentSequenceIsFast ? (TIMING_GOOD_MS / 2) : TIMING_GOOD_MS;
    int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

    bool sequenceFailed = false;

    if (sequence[seqIdx].isFake)
    {
        sequenceFailed = true; 
        setGameMessage("Tripped wire!", 3.0f);
    }
    else if (sequence[seqIdx].dir == pressed)
    {
        int tier = 0;
        if (dt <= currentPerfectMs) tier = 1;
        else if (dt <= currentGoodMs) tier = 2;
        else if (dt <= currentBadMs) tier = 3;
        else tier = -1;

        if (tier == 1 || tier == 2 || tier == 3)
        {
            if (currentSequenceIsHazard && tier == 3)
            {
                sequenceFailed = true;
                setGameMessage("Hazard Failed!", 3.0f);
            }
            else
            {
                sequence[seqIdx].result = tier;
                
                if (tier == 1) {
                    playSFX(SFX_TIMING_PERFECT);
                    spawnParticles(10, player.x, player.y, 0.5f, 0.1f, 1.0f, 0.2f);
                    spawnFloatingText("PERFECT", player.x, player.y + 0.1f, 0.2f, 1.0f, 0.5f, 1.2f);
                }
                else if (tier == 2) {
                    playSFX(SFX_TIMING_GOOD);
                    spawnFloatingText("GOOD", player.x, player.y + 0.1f, 0.8f, 1.0f, 0.2f, 1.0f);
                }
                else if (tier == 3) {
                    playSFX(SFX_TIMING_GOOD); 
                    spawnFloatingText("OKAY", player.x, player.y + 0.1f, 1.0f, 0.8f, 0.2f, 0.8f);
                }
            }
        }
        else sequenceFailed = true; 
    }
    else sequenceFailed = true; 

    if (sequenceFailed)
    {
        handleSequenceFailure();
        return;
    }

    sequence[seqIdx].displayAnim = 1.2f;
    seqIdx++;
    stepWaitingForInput = false;

    if (seqIdx < (int)sequence.size())
    {
        stepStartTimeMs = now + 100;
        stepWaitingForInput = true;
    }
    else
    {
        finishSequenceSuccess();
    }
}

// virus logc and attack
void updateVirus(float dt)
{
    if (!virus.active)
    {
        virusSpawnTimer -= dt;
        if (virusSpawnTimer <= 0.0f)
        {
            // spawn virus at farthest node from player
            int farNode = -1;
            float maxDist = 0.0f;
            for(size_t i=0; i<nodes.size(); ++i) {
                float d = std::hypot(nodes[i].x - player.x, nodes[i].y - player.y);
                if(d > maxDist) { maxDist = d; farNode = (int)i; }
            }
            
            if (farNode != -1) {
                virus.currentNode = farNode;
                virus.x = nodes[farNode].x;
                virus.y = nodes[farNode].y;
                virus.active = true;
                virus.moveTimer = VIRUS_MOVE_INTERVAL;
                virus.isAttacking = false;
                virus.attackTimer = VIRUS_ATTACK_DELAY;
                
                setGameMessage("WARNING: VIRUS DETECTED!", 3.0f);
                playSFX(SFX_VIRUS_WARNING); 
            }
        }
        return;
    }

    // movement
    Node& currentVNode = nodes[virus.currentNode];
    float lerpSpeed = 2.0f;
    virus.x += (currentVNode.x - virus.x) * lerpSpeed * dt;
    virus.y += (currentVNode.y - virus.y) * lerpSpeed * dt;

    if (std::hypot(currentVNode.x - virus.x, currentVNode.y - virus.y) > 0.01f) {
        virus.x += (rand()%100 / 5000.0f - 0.01f); 
        virus.y += (rand()%100 / 5000.0f - 0.01f);
    }

    // attack logic
    bool isOnPlayer = (virus.currentNode == player.currentNode);
    
    if (isOnPlayer)
    {
        virus.isAttacking = true;
        virus.attackTimer -= dt;
        
        if (virus.attackTimer <= 0.0f)
        {
            playerLives--;
            setGameMessage("VIRUS CORRUPTION! NODE LOST.", 2.0f);
            
            // visuals
            spawnParticles(50, player.x, player.y, 2.0f, 1.0f, 0.0f, 0.0f);
            screenShakeTimer = 0.5f;
            playSFX(SFX_TIMING_FAIL);

            if (player.currentNode != -1) {
                nodes[player.currentNode].visited = false; 
            }

            if (playerLives <= 0) {
                isGameOver = true;
                setGameMessage("TERMINATED BY VIRUS.", 1000.0f);
            } else {
                virus.active = false;
                virusSpawnTimer = 60.0f - ((currentLevel - 1) * 5.0f); 
            }
        }
    }
    else
    {
        virus.isAttacking = false;
        virus.attackTimer = VIRUS_ATTACK_DELAY;
        
        virus.moveTimer -= dt;
        if (virus.moveTimer <= 0.0f)
        {
            virus.moveTimer = VIRUS_MOVE_INTERVAL;

            int bestNext = -1;
            int shortestDist = 9999;

            std::vector<int> dist(nodes.size(), 9999);
            std::queue<int> q;
            
            int target = player.moving ? player.moveTo : player.currentNode;
            if (target < 0 || target >= nodes.size()) target = 0; 

            q.push(target);
            dist[target] = 0;
            
            while(!q.empty()) {
                int u = q.front(); q.pop();
                if (dist[u] > shortestDist) continue; 

                for(int v : nodes[u].links) {
                    if(dist[v] == 9999) {
                        dist[v] = dist[u] + 1;
                        q.push(v);
                    }
                }
            }

            for(int neighbor : nodes[virus.currentNode].links) {
                if(dist[neighbor] < shortestDist) {
                    shortestDist = dist[neighbor];
                    bestNext = neighbor;
                }
            }
    
            if (bestNext == -1 && !nodes[virus.currentNode].links.empty()) {
                 bestNext = nodes[virus.currentNode].links[rng() % nodes[virus.currentNode].links.size()];
            }

            if (bestNext != -1) {
                virus.currentNode = bestNext;
              
                float dx = nodes[virus.currentNode].x - cameraX;
                float dy = nodes[virus.currentNode].y - cameraY;
                if (fabs(dx) < screenAspect && fabs(dy) < 1.0f) {
                    playSFX(SFX_MOVE_HIGHLIGHT);
                }
            }
        }
    }
}

void updateGame()
{
    if (isGameOver)
        return;

    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = 0.016f; 

    if (inputCooldown > 0.0f) inputCooldown -= dt;

    updateParticles(dt);
    updateShockwaves(dt);
    
    updateFloatingTexts(dt);
    
    if (!isLevelTransitioning)
        updateVirus(dt);

    if (isLevelTransitioning)
    {
        levelTransitionTimer -= dt;
        
        if (rng() % 2 == 0) {
            spawnParticles(2, player.x, player.y, 5.0f, 0.2f, 1.0f, 1.0f); 
        }

        if (levelTransitionTimer <= 0.0f)
        {
            currentLevel++;
            int numNodes = std::min(30, 8 + currentLevel);
            int numLocks = std::min(5, 1 + (currentLevel / 2));
            int maxLinks = std::min(4, 2 + currentLevel / 3);
            playerLives = STARTING_LIVES + (currentLevel / 3); 
            
            makeMap(numNodes, 1, maxLinks, numLocks);
            
            inSequence = false;
            sequence.clear();
            player.targetNode = -1;
            finalNodeSpawned = false;
            gameWon = false;
            isGameOver = false;
            finalNodeIndex = -1;
            gameInProgress = true;
            isLevelTransitioning = false;
           
            virus.active = false;
            float spawnTime = 60.0f - ((currentLevel - 1) * 5.0f);
            virusSpawnTimer = std::max(10.0f, spawnTime);
            virus.isAttacking = false;
            
            floatingTexts.clear();

            setGameMessage("Level " + std::to_string(currentLevel) + "!", 3.0f);
        }
        return; 
    }

    float targetCamX = player.x;
    float targetCamY = player.y;

    if (currentGameState == STATE_PLAYING)
    {
        float peekZone = 0.1f; // 10% of screen edge
        float peekAmount = 0.5f;
        float mouseX_norm = (float)g_mouseX / windowW;
        float mouseY_norm = 1.0f - (float)g_mouseY / windowH;

        float peekX = 0.0f, peekY = 0.0f;

        if (mouseX_norm < peekZone)
            peekX = -peekAmount * (1.0f - mouseX_norm / peekZone);
        else if (mouseX_norm > 1.0f - peekZone)
            peekX = peekAmount * (1.0f - (1.0f - mouseX_norm) / peekZone);

        if (mouseY_norm < peekZone)
            peekY = -peekAmount * (1.0f - mouseY_norm / peekZone);
        else if (mouseY_norm > 1.0f - peekZone)
            peekY = peekAmount * (1.0f - (1.0f - mouseY_norm) / peekZone);

        targetCamX += peekX;
        targetCamY += peekY;
    }


    if (player.moving && player.moveTo != -1) {
        float leadFactor = 0.3f;
        targetCamX = player.x * (1.0f - leadFactor) + nodes[player.moveTo].x * leadFactor;
        targetCamY = player.y * (1.0f - leadFactor) + nodes[player.moveTo].y * leadFactor;
    }
    else if (!player.moving && !inSequence && player.highlightedNeighbor != -1) {
        clampHighlightedNeighbor();
        if (player.highlightedNeighbor >= 0) {
            int neighborIdx = nodes[player.currentNode].links[player.highlightedNeighbor];
            Node& target = nodes[neighborIdx];
            targetCamX = player.x * 0.7f + target.x * 0.3f;
            targetCamY = player.y * 0.7f + target.y * 0.3f;
        }
    }

    float camSpeed = 0.08f; 
    cameraX += (targetCamX - cameraX) * camSpeed;
    cameraY += (targetCamY - cameraY) * camSpeed;


    if (screenShakeTimer > 0.0f)
        screenShakeTimer -= dt;

    if (player.moving || inSequence || displayingPassword || currentGameState != STATE_PLAYING || player.highlightedNeighbor == -1)
    {
        player.avatarAngle = fmod(now * PLAYER_AVATAR_SPIN_SPEED / 1000.0f, 360.0f);
    }
    else
    {
        clampHighlightedNeighbor();
        if (player.highlightedNeighbor != -1)
        {
            int neighborIdx = nodes[player.currentNode].links[player.highlightedNeighbor];
            Node& target = nodes[neighborIdx];
            float dx = target.x - player.x;
            float dy = target.y - player.y;
            float targetAngle = atan2(dy, dx) * 180.0f / 3.1415926f;

            float diff = targetAngle - player.avatarAngle;
            while (diff < -180.0f)
                diff += 360.0f;
            while (diff > 180.0f)
                diff -= 360.0f;
            player.avatarAngle += diff * PLAYER_AVATAR_TURN_SPEED;
        }
    }

    if (gameWon)
    {
        if (rng() % 3 == 0)
            spawnParticles(5, player.x, player.y, 3.0f, 0.2f, 1.0f, 0.3f);
    }

    for (auto &a : sequence)
    {
        if (a.displayAnim > 0.0f)
        {
            a.displayAnim -= 0.02f;
            if (a.displayAnim < 0.0f)
                a.displayAnim = 0.0f;
        }
    }

    if (gameMessageTimer > 0.0f)
    {
        gameMessageTimer -= dt;
        if (gameMessageTimer <= 0.0f)
            gameMessage = "";
    }

    if (!finalNodeSpawned && !gameWon)
    {
        bool allVisited = true;
        for (const auto &n : nodes)
        {
            if (n.isFinal)
                continue;
            if (!n.visited)
            {
                allVisited = false;
                break;
            }
        }
        if (allVisited)
            spawnFinalNode();
    }

    int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

    if (inSequence && !isPasswordInput && stepWaitingForInput)
    {
        int dt_ms = now - stepStartTimeMs;

        if (seqIdx < (int)sequence.size() && sequence[seqIdx].isFake)
        {
            if (dt_ms > currentBadMs + 100)
            {
                sequence[seqIdx].result = 1;
                sequence[seqIdx].displayAnim = 1.2f;
                playSFX(SFX_MOVE_HIGHLIGHT); 

                seqIdx++;
                stepWaitingForInput = false;

                if (seqIdx < (int)sequence.size())
                {
                    stepStartTimeMs = now + 100;
                    stepWaitingForInput = true;
                }
                else
                {
                    finishSequenceSuccess();
                }
                return;
            }
        }
        else if (dt_ms > currentBadMs + 300)
        {
            handleSequenceFailure();
        }
    }

    if (player.moving)
    {
        float t = (now - player.moveStartTimeMs) / (float)MOVE_DURATION_MS;
        
        if (t >= 1.0f)
        {
            player.moving = false;
            player.x = nodes[player.moveTo].x;
            player.y = nodes[player.moveTo].y;
            playSFX(SFX_MOVE_SUCCESS); 

            if (player.moveTo == finalNodeIndex && nodes[player.moveTo].isFinal)
            {
                gameWon = true;
                gameMessage = loreSnippets[currentLevel % loreSnippets.size()];
                gameMessageTimer = 1000.0f;
                gameMessageMaxTimer = 1000.0f;
                playSFX(SFX_TIMING_PERFECT); 
            }
        }
        else
        {
            float s = t * t * (3 - 2 * t); 
            player.x = nodes[player.moveFrom].x * (1.0f - s) + nodes[player.moveTo].x * s;
            player.y = nodes[player.moveFrom].y * (1.0f - s) + nodes[player.moveTo].y * s;
        }
    }

    if (currentGameState == STATE_PLAYING)
    {
        player.trail.push_back({player.x, player.y});
        if (player.trail.size() > 20)
        {
            player.trail.erase(player.trail.begin());
        }
    }
}

void initLore()
{
    loreSnippets.push_back("ANOMALY 1 SOLVED. ...Firewall integrity 70%...");
    loreSnippets.push_back("ANOMALY 2 SOLVED. ...Log: The anomaly seems to be... learning...?");
    loreSnippets.push_back("ANOMALY 3 SOLVED. ...Re-routing main power. We can't let it out...");
    loreSnippets.push_back("ANOMALY 4 SOLVED. ...Patch failed. Again. It's too fast...");
    loreSnippets.push_back("ANOMALY 5 SOLVED. ...Mainframe patch... THISPATCHED...");
    loreSnippets.push_back("ANOMALY 6 SOLVED. ...It's not an anomaly. It's a signal...");
    loreSnippets.push_back("ANOMALY 7 SOLVED. ...Who is on the other side? What do they want?...");
    loreSnippets.push_back("ANOMALY 8 SOLVED. ...System compromise at 90%. I can't stop it...");
    loreSnippets.push_back("ANOMALY 9 SOLVED. ...The core is open. It's free...");
    loreSnippets.push_back("ANOMALY 10 SOLVED. ...Hello, world.");
}