#include "game.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <GL/freeglut.h>

void drawString(const char* s, float x, float y, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glLineWidth(2.0f);
    for (const char* p = s; *p; ++p)
    {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *p);
    }
    glPopMatrix();
}

void drawCircle(float cx, float cy, float r, int seg)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; i++)
    {
        float a = (float)i / seg * 2.0f * 3.1415926f;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void drawRing(float cx, float cy, float ro, float ri, int seg)
{
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= seg; i++)
    {
        float a = (float)i / seg * 2.0f * 3.1415926f;
        glVertex2f(cx + cosf(a) * ro, cy + sinf(a) * ro);
        glVertex2f(cx + cosf(a) * ri, cy + sinf(a) * ri);
    }
    glEnd();
}

void drawQuad(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void drawTexturedQuad(float cx, float cy, float w, float h, float alpha)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, uiTex);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(cx - w / 2, cy - h / 2);
    glTexCoord2f(1, 0);
    glVertex2f(cx + w / 2, cy - h / 2);
    glTexCoord2f(1, 1);
    glVertex2f(cx + w / 2, cy + h / 2);
    glTexCoord2f(0, 1);
    glVertex2f(cx - w / 2, cy + h / 2);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawKeyButton(float x, float y, float w, float h, const char* label, bool pressed)
{
    if (pressed) glColor4f(0.6f, 0.8f, 1.0f, 0.9f); 
    else glColor4f(0.1f, 0.12f, 0.15f, 0.8f);       
    
    drawQuad(x - w/2, y - h/2, x + w/2, y + h/2);

    glLineWidth(2.0f);
    if (pressed) glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    else glColor4f(0.4f, 0.5f, 0.6f, 1.0f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(x - w/2, y - h/2);
    glVertex2f(x + w/2, y - h/2);
    glVertex2f(x + w/2, y + h/2);
    glVertex2f(x - w/2, y + h/2);
    glEnd();

    if (pressed) glColor3f(0.0f, 0.0f, 0.0f);
    else glColor3f(1.0f, 1.0f, 1.0f);

    float charSize = 104.76f; 
    float textScale = h * 0.004f; 
    float textLen = strlen(label) * charSize * textScale;

    drawString(label, x - textLen * 0.5f, y - (119.05f * textScale) * 0.3f, textScale);
}

void drawEnergyIcon(float cx, float cy, float radius, float pulseAlpha)
{
    glColor4f(0.2f, 0.8f, 1.0f, 0.5f * pulseAlpha);
    drawCircle(cx, cy, radius * (1.0f + 0.3f * pulseAlpha));
    glColor3f(0.2f, 0.8f, 1.0f);
    drawCircle(cx, cy, radius * 0.6f);
    glColor3f(0.8f, 1.0f, 1.0f);
    drawCircle(cx, cy, radius * 0.2f);
}

void drawArrowShape(float x, float y, ArrowDir dir, int result, float size, float animScale, bool isFake)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    float ang = 0;
    if (dir == ARROW_RIGHT) ang = 0;
    else if (dir == ARROW_UP) ang = 90;
    else if (dir == ARROW_LEFT) ang = 180;
    else ang = -90;
    glRotatef(ang, 0, 0, 1);
    glScalef(size * animScale, size * animScale, 1);

    if (isFake && result == 0) glColor3f(1.0f, 0.2f, 0.2f); 
    else if (result == 1) glColor3f(0.1f, 1.0f, 0.2f); 
    else if (result == 2) glColor3f(0.4f, 0.8f, 0.2f); 
    else if (result == 3) glColor3f(1.0f, 0.7f, 0.2f); 
    else if (result == -1) glColor3f(1.0f, 0.2f, 0.2f); 
    else glColor3f(0.95f, 0.95f, 0.95f); 

    glBegin(GL_TRIANGLES);
    glVertex2f(0.9f, 0.0f);
    glVertex2f(-0.4f, 0.55f);
    glVertex2f(-0.4f, -0.55f);
    glEnd();
    glPopMatrix();
}

void drawAvatarShape(float x, float y, float radius, float angle)
{
    int sides = PLAYER_AVATAR_SIDES;
    float spinAngle = (angle == -1.0f) ? 
        fmod(glutGet(GLUT_ELAPSED_TIME) * PLAYER_AVATAR_SPIN_SPEED / 1000.0f, 360.0f) : angle;

    glColor3f(0.9f, 0.2f, 0.9f);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(spinAngle, 0, 0, 1);
    glBegin(GL_POLYGON);
    for (int i = 0; i < sides; i++)
    {
        float a = i / (float)sides * 2.0f * 3.1415926f;
        glVertex2f(cosf(a) * radius, sinf(a) * radius);
    }
    glEnd();
    glPopMatrix();
}

void drawNodeIcon(NodeIcon icon, float x, float y)
{
    float iconSize = 0.025f;
    float lineSize = 0.008f;

    switch (icon)
    {
    case ICON_REWARD:
        glColor3f(1.0f, 0.9f, 0.3f);
        drawQuad(x - iconSize, y - lineSize / 2, x + iconSize, y + lineSize / 2);
        drawQuad(x - lineSize / 2, y - iconSize, x + lineSize / 2, y + iconSize);
        break;
    case ICON_HAZARD:
        glColor3f(0.6f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(45, 0, 0, 1);
        drawQuad(-iconSize, -lineSize / 2, iconSize, lineSize / 2);
        drawQuad(-lineSize / 2, -iconSize, lineSize / 2, iconSize);
        glPopMatrix();
        break;
    case ICON_FAST:
        glColor3f(0.8f, 0.6f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x + 0.005f, y + 0.015f);
        glVertex2f(x + 0.02f, y);
        glVertex2f(x + 0.005f, y - 0.015f);
        glVertex2f(x - 0.015f, y + 0.015f);
        glVertex2f(x + 0.00f, y);
        glVertex2f(x - 0.015f, y - 0.015f);
        glEnd();
        break;
    case ICON_KEY:
        glColor3f(0.1f, 0.1f, 0.1f);
        drawCircle(x - 0.01f, y + 0.01f, 0.015f);
        drawQuad(x - 0.01f, y - 0.003f, x + 0.02f, y + 0.003f);
        drawQuad(x + 0.015f, y - 0.01f, x + 0.02f, y + 0.003f);
        break;
    case ICON_LOCK:
        glColor3f(1.0f, 0.3f, 0.3f);
        drawQuad(x - 0.015f, y - 0.02f, x + 0.015f, y + 0.005f);
        drawRing(x, y + 0.005f, 0.01f, 0.007f, 16);
        break;
    case ICON_WARP:
        glColor3f(0.8f, 0.3f, 1.0f); 
        glPushMatrix();
        glTranslatef(x, y, 0);
        drawQuad(-iconSize, -lineSize / 2, iconSize, lineSize / 2);
        drawQuad(-lineSize / 2, -iconSize, lineSize / 2, iconSize);
        glRotatef(45, 0, 0, 1);
        drawQuad(-iconSize, -lineSize / 2, iconSize, lineSize / 2);
        drawQuad(-lineSize / 2, -iconSize, lineSize / 2, iconSize);
        glPopMatrix();
        break;
    default:
        break;
    }
}

GLuint makeProceduralUITexture(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 4);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            float fx = (float)x / (w - 1), fy = (float)y / (h - 1);
            unsigned char r = (unsigned char)(30 + 80 * fy + 30 * fx);
            unsigned char g = (unsigned char)(50 + 80 * fy);
            unsigned char b = (unsigned char)(70 + 50 * fx);
            int gx = (x / 8) % 2, gy = (y / 8) % 2;
            int add = (gx ^ gy) ? 6 : 0;
            int idx = (y * w + x) * 4;
            pixels[idx + 0] = (unsigned char)std::min(255, r + add);
            pixels[idx + 1] = (unsigned char)std::min(255, g + add);
            pixels[idx + 2] = (unsigned char)std::min(255, b + add);
            pixels[idx + 3] = 255;
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return tex;
}

void spawnParticles(int count, float x, float y, float initialSpeed, float r, float g, float b)
{
    std::uniform_real_distribution<float> angleDist(0, 2.0f * 3.1415926f);
    std::uniform_real_distribution<float> speedDist(0.1f, initialSpeed);
    std::uniform_real_distribution<float> lifeDist(1.0f, 2.0f);

    for (int i = 0; i < count; ++i)
    {
        Particle p;
        p.x = x;
        p.y = y;

        float angle = angleDist(rng);
        float speed = speedDist(rng);
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;

        p.life = p.initialLife = lifeDist(rng);
        p.r = r;
        p.g = g;
        p.b = b;
        particles.push_back(p);
    }
}

void updateParticles(float dt)
{
    for (size_t i = 0; i < particles.size(); )
    {
        Particle& p = particles[i];
        p.life -= dt;

        if (p.life <= 0)
        {
            particles[i] = particles.back();
            particles.pop_back();
        }
        else
        {
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.vy += PARTICLE_GRAVITY * dt;
            p.vx *= PARTICLE_DRAG;
            p.vy *= PARTICLE_DRAG;
            i++;
        }
    }
}

void drawParticles()
{
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.0f);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_POINTS);
    for (const auto& p : particles)
    {
        float alpha = p.life / p.initialLife;
        glColor4f(p.r, p.g, p.b, alpha);
        glVertex2f(p.x, p.y);
    }
    glEnd();
    glPointSize(1.0f);
    glDisable(GL_POINT_SMOOTH);
}

void drawFloatingTexts()
{
    for (const auto& ft : floatingTexts)
    {
        float alpha = ft.life; 
        glColor4f(ft.r, ft.g, ft.b, alpha);
        
        float s = ft.scale * FONT_SCALE_LARGE;
        if (ft.life > 0.8f) s *= 1.2f; 
        
        float width = ft.text.length() * s * 300.0f; 
        drawString(ft.text.c_str(), ft.x - (width * 0.0005f), ft.y, s);
    }
}

void drawBackgroundGrid()
{
    float time = glutGet(GLUT_ELAPSED_TIME) / 5000.0f;
    float scrollX = fmod(cameraX, 0.12f);
    float scrollY = fmod(cameraY, 0.12f);
    float extra = 0.5f; 

    glPushMatrix();
    glTranslatef(cameraX - scrollX, cameraY - scrollY, 0);

    glColor4f(0.10f, 0.12f, 0.16f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -screenAspect - extra; x <= screenAspect + extra; x += 0.12f)
    {
        glVertex2f(x, -1.0f - extra);
        glVertex2f(x, 1.0f + extra);
    }
    float y_offset_far = fmod(time * 0.1f, 0.12f);
    for (float y = -1.0f - extra; y <= 1.0f + extra; y += 0.12f)
    {
        glVertex2f(-screenAspect - extra, y + y_offset_far);
        glVertex2f(screenAspect + extra, y + y_offset_far);
    }
    glEnd();

    glColor4f(0.15f, 0.18f, 0.24f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    float y_offset_near = fmod(time * 0.3f, 0.12f);
    for (float y = -1.0f - extra; y <= 1.01f + extra; y += 0.12f)
    {
        glVertex2f(-screenAspect - extra, y + y_offset_near - 0.06f);
        glVertex2f(screenAspect + extra, y + y_offset_near - 0.06f);
    }
    glEnd();
    glPopMatrix();
}

void drawMap()
{
    drawBackgroundGrid();

    glLineWidth(3.0f);
    float globalPulse = 0.8f + 0.2f * (1.0f + sinf(glutGet(GLUT_ELAPSED_TIME) * 0.005f));

    int highlightedNodeIndex = -1;
    if (!nodes.empty() && player.currentNode >= 0 && player.currentNode < (int)nodes.size() && !inSequence && !player.moving && !displayingPassword)
    {
        clampHighlightedNeighbor();
        if (player.highlightedNeighbor >= 0)
            highlightedNodeIndex = nodes[player.currentNode].links[player.highlightedNeighbor];
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        for (int j : nodes[i].links)
        {
            if (j < (int)i) continue;

            bool isPlayerLink = (i == player.currentNode || j == player.currentNode) && !player.moving && !inSequence && !displayingPassword;
            bool isHighlightLink = (highlightedNodeIndex != -1) && ((i == player.currentNode && j == highlightedNodeIndex) || (i == highlightedNodeIndex && j == player.currentNode)) && !player.moving && !inSequence && !displayingPassword;
            bool isWarpLink = (nodes[i].isWarpNode || nodes[j].isWarpNode);

           if (isHighlightLink)
            {
                if (nodes[highlightedNodeIndex].visited)
                {
                    glColor3f(1.0f * globalPulse, 1.0f * globalPulse, 0.0f);
                }
                else
                {
                    glColor3f(0.4f * globalPulse, 1.0f * globalPulse, 0.5f * globalPulse);
                }
            }
            else if (isPlayerLink)
                glColor3f(0.6f * globalPulse, 0.75f * globalPulse, 0.9f * globalPulse);
            else if (isWarpLink)
                glColor3f(0.5f, 0.3f, 0.6f);
            else
                glColor3f(0.6f, 0.75f, 0.9f);

            glBegin(GL_LINES);
            glVertex2f(nodes[i].x, nodes[i].y);
            glVertex2f(nodes[j].x, nodes[j].y);
            glEnd();
        }
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        float x = nodes[i].x, y = nodes[i].y;

        if (nodes[i].isTargetOfKey && nodes[i].isLocked)
        {
            float pulse = 0.5f + 0.5f * (1.0f + sinf(glutGet(GLUT_ELAPSED_TIME) * 0.008f));
            glColor4f(1.0f, 1.0f, 1.0f, pulse);
            drawRing(x, y, NODE_RING_KEY_PULSE_OUT, NODE_RING_KEY_PULSE_IN);
        }

        if (nodes[i].isFinal)
        {
            glColor3f(1.0f, 0.8f, 0.1f);
            drawRing(x, y, NODE_RING_FINAL_OUT, NODE_RING_FINAL_IN);
            glColor3f(0.95f, 0.85f, 0.2f);
            drawCircle(x, y, NODE_RADIUS_VISITED);
            glColor3f(0.05f, 0.05f, 0.05f);
            drawString("FINISH", x - 0.07f, y - 0.01f, FONT_SCALE_LARGE);
            continue;
        }

        if ((int)i == player.currentNode)
            glColor3f(0.12f, 0.45f, 0.85f), drawRing(x, y, NODE_RING_CURRENT_OUT, NODE_RING_CURRENT_IN);

        if (nodes[i].visited)
            glColor3f(0.22f, 0.45f, 0.85f);
        else if (nodes[i].type == NODE_HAZARD)
            glColor3f(1.0f, 0.3f, 0.3f);
        else if (nodes[i].type == NODE_FAST)
            glColor3f(1.0f, 0.8f, 0.2f);
        else if (nodes[i].type == NODE_WARP)
            glColor3f(0.8f, 0.3f, 1.0f);
        else
            glColor3f(0.95f, 0.95f, 0.95f);
        
        drawCircle(x, y, nodes[i].visited ? NODE_RADIUS_VISITED : NODE_RADIUS_UNVISITED);

        NodeIcon icon = ICON_NONE;
        if (nodes[i].type == NODE_REWARD) icon = ICON_REWARD;
        else if (nodes[i].type == NODE_HAZARD) icon = ICON_HAZARD;
        else if (nodes[i].type == NODE_FAST) icon = ICON_FAST;
        else if (nodes[i].type == NODE_WARP) icon = ICON_WARP;

        if (nodes[i].isKey) icon = ICON_KEY;
        else if (nodes[i].isLocked) icon = ICON_LOCK;

        if (icon != ICON_NONE)
            drawNodeIcon(icon, x, y);
    }

    if (!nodes.empty() && player.currentNode >= 0 && player.currentNode < (int)nodes.size() && !inSequence && !player.moving && !displayingPassword)
    {
        if (highlightedNodeIndex != -1) 
        {
            glColor3f(0.4f, 1.0f, 0.5f);
            drawRing(nodes[highlightedNodeIndex].x, nodes[highlightedNodeIndex].y, NODE_RING_HIGHLIGHT_OUT, NODE_RING_HIGHLIGHT_IN);
        }
        else if (player.highlightedNeighbor == -1) 
        {
            Node& currNode = nodes[player.currentNode];
            if (currNode.isKey && revealedPasswords.count(currNode.lockNode))
            {
                glColor3f(0.9f, 0.9f, 0.2f); 
                drawRing(currNode.x, currNode.y, NODE_RING_HIGHLIGHT_OUT, NODE_RING_HIGHLIGHT_IN);
            }
        }
    }
}

void drawAvatar(float x, float y)
{
    drawAvatarShape(x, y, PLAYER_AVATAR_RADIUS, player.avatarAngle);
}

void drawVirus(float x, float y)
{
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.02f; 
    float pulse = 1.0f + 0.15f * fabs(sinf(time)); 
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(pulse, pulse, 1);
    
    // aura
    glColor4f(1.0f, 0.0f, 0.0f, 0.6f);
    drawCircle(0, 0, 0.06f);
    
    // core
    glColor3f(0.2f, 0.0f, 0.0f);
    drawCircle(0, 0, 0.03f);
    
    // spikes
    glBegin(GL_TRIANGLES);
    glColor3f(0.8f, 0.0f, 0.0f);
    for(int i=0; i<8; ++i) {
        float a = i * 3.14159f / 4.0f + time * 0.5f; 
        glVertex2f(cos(a)*0.07f, sin(a)*0.07f);
        glVertex2f(cos(a+0.2f)*0.03f, sin(a+0.2f)*0.03f);
        glVertex2f(cos(a-0.2f)*0.03f, sin(a-0.2f)*0.03f);
    }
    glEnd();
    glPopMatrix();
}

void drawOffScreenIndicator(float tx, float ty, NodeIcon icon, NodeType type)
{
    float relX = tx - cameraX;
    float relY = ty - cameraY;

    float margin = 0.15f;
    float boundX = screenAspect - margin;
    float boundY = 1.0f - margin;

    if (fabs(relX) < boundX && fabs(relY) < boundY) return; 

    float angle = atan2(relY, relX);
    float slope = tan(angle);
    float edgeX, edgeY;
    
    if (fabs(relX) > boundX) 
    {
        edgeX = (relX > 0) ? boundX : -boundX;
        edgeY = edgeX * slope;
        
        if (fabs(edgeY) > boundY)
        {
            edgeY = (relY > 0) ? boundY : -boundY;
            edgeX = edgeY / slope;
        }
    }
    else 
    {
        edgeY = (relY > 0) ? boundY : -boundY;
        edgeX = edgeY / slope;
    }

    float time = glutGet(GLUT_ELAPSED_TIME) * 0.006f; 
    float bobAmp = 0.025f; 
    float bobOffset = sinf(time) * bobAmp;

    float bobX = cos(angle) * bobOffset;
    float bobY = sin(angle) * bobOffset;

    float drawX = cameraX + edgeX + bobX;
    float drawY = cameraY + edgeY + bobY;

    float degAngle = angle * 180.0f / 3.14159f;
    
    glPushMatrix();
    glTranslatef(drawX, drawY, 0);
    glRotatef(degAngle, 0, 0, 1);
    
    if (icon == ICON_NONE && type == NODE_HAZARD) glColor3f(1.0f, 0.0f, 0.0f); 
    else glColor3f(1.0f, 1.0f, 1.0f); 

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.06f, 0.0f);
    glVertex2f(-0.03f, 0.05f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(-0.03f, -0.05f);
    glEnd();
    
    glPopMatrix();

    float iconOffset = 0.04f; 
    float iconX = drawX - cos(angle) * iconOffset;
    float iconY = drawY - sin(angle) * iconOffset;

    if (type == NODE_HAZARD) glColor3f(1.0f, 0.2f, 0.2f);
    else if (type == NODE_REWARD) glColor3f(1.0f, 0.9f, 0.3f);
    else if (type == NODE_WARP) glColor3f(0.8f, 0.3f, 1.0f);
    else glColor3f(0.3f, 0.6f, 1.0f); 
    
    drawCircle(iconX, iconY, 0.035f, 16);
    
    if (icon != ICON_NONE)
        drawNodeIcon(icon, iconX, iconY);
}

void drawMiniMap()
{
    float mmW = MINIMAP_SIZE;
    float mmH = MINIMAP_SIZE * (1.0f / screenAspect); 
    if (mmH > 0.5f) mmH = 0.5f; 

    float mmX = screenAspect - mmW / 2.0f - MINIMAP_OFFSET_X;
    float mmY = -1.0f + mmH / 2.0f + MINIMAP_OFFSET_Y;

    glColor4f(0.1f, 0.3f, 0.6f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(mmX - mmW/2 - 0.01f, mmY - mmH/2 - 0.01f);
    glVertex2f(mmX + mmW/2 + 0.01f, mmY - mmH/2 - 0.01f);
    glVertex2f(mmX + mmW/2 + 0.01f, mmY + mmH/2 + 0.01f);
    glVertex2f(mmX - mmW/2 - 0.01f, mmY + mmH/2 + 0.01f);
    glEnd();

    glColor4f(0.0f, 0.05f, 0.1f, MINIMAP_OPACITY);
    drawQuad(mmX - mmW/2, mmY - mmH/2, mmX + mmW/2, mmY + mmH/2);
    
    glColor4f(0.1f, 0.2f, 0.3f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(mmX, mmY - mmH/2);
    glVertex2f(mmX, mmY + mmH/2);
    glVertex2f(mmX - mmW/2, mmY);
    glVertex2f(mmX + mmW/2, mmY);
    glEnd();

    float scaleX = mmW / (MAP_WIDTH * 2.0f + 0.5f); 
    float scaleY = mmH / (MAP_HEIGHT * 2.0f + 0.5f);
    float scale = std::min(scaleX, scaleY);

    glLineWidth(1.0f);
    glColor4f(0.3f, 0.4f, 0.5f, 0.6f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (int neighbor : nodes[i].links) {
            if (neighbor < (int)i) continue;
            float x1 = mmX + nodes[i].x * scale;
            float y1 = mmY + nodes[i].y * scale;
            float x2 = mmX + nodes[neighbor].x * scale;
            float y2 = mmY + nodes[neighbor].y * scale;
            glVertex2f(x1, y1);
            glVertex2f(x2, y2);
        }
    }
    glEnd();

    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < nodes.size(); ++i)
    {
        const auto& n = nodes[i];
        bool isTarget = false;
        if (player.highlightedNeighbor >= 0) {
             int targetId = nodes[player.currentNode].links[player.highlightedNeighbor];
             if (i == targetId) isTarget = true;
        }

        if (isTarget) {
             float flash = sinf(glutGet(GLUT_ELAPSED_TIME) * 0.01f);
             glColor3f(1.0f, 0.5f + 0.5f*flash, 0.5f + 0.5f*flash); 
        }
        else if (n.isFinal) glColor3f(1.0f, 0.8f, 0.0f);
        else if (n.visited) glColor3f(0.2f, 0.5f, 0.9f);
        else glColor3f(0.3f, 0.3f, 0.3f); 

        float nx = mmX + n.x * scale;
        float ny = mmY + n.y * scale;
        glVertex2f(nx, ny);
    }
    glEnd();
    
    if (virus.active) {
        glPointSize(8.0f); 
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_POINTS);
        glVertex2f(mmX + virus.x * scale, mmY + virus.y * scale);
        glEnd();
    }

    if (player.highlightedNeighbor >= 0) {
         int targetId = nodes[player.currentNode].links[player.highlightedNeighbor];
         Node& t = nodes[targetId];
         float tx = mmX + t.x * scale;
         float ty = mmY + t.y * scale;
         glColor3f(1.0f, 1.0f, 0.0f);
         drawRing(tx, ty, 0.02f, 0.015f, 8);
    }

    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.2f, 1.0f); 
    glVertex2f(mmX + player.x * scale, mmY + player.y * scale);
    glEnd();

    float viewW = screenAspect * 2.0f * scale;
    float viewH = 2.0f * scale;
    float cx = mmX + cameraX * scale;
    float cy = mmY + cameraY * scale;
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - viewW/2, cy - viewH/2);
    glVertex2f(cx + viewW/2, cy - viewH/2);
    glVertex2f(cx + viewW/2, cy + viewH/2);
    glVertex2f(cx - viewW/2, cy + viewH/2);
    glEnd();
}

void drawPasswordDisplay()
{
    float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);

    const char* title = justRevealedPassword ? "DECRYPTER PATH ACQUIRED" : "VIEWING PASSWORD";
    drawString(title, cx - 0.22f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);

    float arrowY = cy - 0.01f;
    int numArrows = (int)sequence.size();
    if (numArrows == 0) return;

    float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
    float startX = cx - totalWidth / 2.0f;

    for (size_t i = 0; i < numArrows; i++)
    {
        float px = startX + i * ARROW_UI_X_SPACING;
        drawArrowShape(px, arrowY, sequence[i].dir, 0, ARROW_DISPLAY_SIZE, 1.0f);
    }

    drawString("Press SPACE to Continue", cx - 0.2f, cy - 0.08f, FONT_SCALE_MEDIUM);
}

void drawSequenceUI()
{
    float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    if (isPasswordInput)
    {
        glColor3f(0.9f, 0.2f, 0.2f);
        drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
        glColor3f(1, 1, 1);
        drawString("ENTER PASSWORD", cx - 0.18f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);

        float arrowY = cy - 0.01f;
        int numArrows = (int)nodes[player.targetNode].password.size();
        if (numArrows == 0) return;

        float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
        float startX = cx - totalWidth / 2.0f;

        for (size_t i = 0; i < numArrows; i++)
        {
            float px = startX + i * ARROW_UI_X_SPACING;
            if (i < sequence.size()) 
            {
                drawArrowShape(px, arrowY, sequence[i].dir, sequence[i].result, ARROW_DISPLAY_SIZE, sequence[i].displayAnim);
            }
            else 
            {
                glColor3f(0.5f, 0.5f, 0.5f);
                drawCircle(px, arrowY, 0.02f);
            }
        }
        drawString("Press SPACE to Cancel", cx - 0.2f, cy - 0.08f, FONT_SCALE_MEDIUM);
        return;
    }

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);

    if (currentSequenceIsHazard)
    {
        std::string hazardText = "HAZARD: " + std::to_string(totalHazardSequences - hazardSequencesRemaining + 1) + " / " + std::to_string(totalHazardSequences);
        drawString(hazardText.c_str(), cx - 0.15f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);
    }
    else
        drawString("BUILD PATH", cx - 0.15f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);


    float arrowY = cy - 0.01f;
    int numArrows = (int)sequence.size();
    if (numArrows == 0) return;

    float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
    float startX = cx - totalWidth / 2.0f;

    for (size_t i = 0; i < numArrows; i++)
    {
        float px = startX + i * ARROW_UI_X_SPACING;
        float animScale = 1.0f + 0.12f * sequence[i].displayAnim;

        if (i == seqIdx && stepWaitingForInput)
        {
            float pulse = 1.0f + 0.1f * (1.0f + sinf(glutGet(GLUT_ELAPSED_TIME) * 0.008f));
            animScale *= pulse;
        }

        if (!sequence[i].chordDirs.empty() && sequence[i].chordDirs.size() > 1)
        {
            float span = 0.06f;
            float leftX = px - span * 0.5f;
            float rightX = px + span * 0.5f;
            float ringR = 0.035f;
            float ringY = arrowY - 0.08f; 

            // base soft halo
            glColor4f(0.06f, 0.45f, 0.9f, 0.08f);
            drawCircle(leftX, ringY, ringR * 1.1f, 24);
            drawCircle(rightX, ringY, ringR * 1.1f, 24);

            // pressed states
            bool leftPressed = false, rightPressed = false;
            if (sequence[i].chordPressed.size() == sequence[i].chordDirs.size())
            {
                leftPressed = sequence[i].chordPressed[0];
                rightPressed = sequence[i].chordPressed[1];
            }

            // rotating rings
            float now = glutGet(GLUT_ELAPSED_TIME) * 0.06f;
            float rotA = fmod(now, 360.0f);
            float rotB = fmod(-now * 1.3f, 360.0f);

            // left ring
            glPushMatrix();
            glTranslatef(leftX, ringY, 0);
            glRotatef(rotA, 0, 0, 1);
            if (leftPressed) glColor4f(0.08f, 0.9f, 1.0f, 0.95f); else glColor4f(0.12f, 0.7f, 1.0f, 0.65f);
            drawRing(0.0f, 0.0f, ringR, ringR - 0.008f, 28);
            glPopMatrix();

            // right ring 
            glPushMatrix();
            glTranslatef(rightX, ringY, 0);
            glRotatef(rotB, 0, 0, 1);
            if (rightPressed) glColor4f(0.08f, 0.9f, 1.0f, 0.95f); else glColor4f(0.12f, 0.7f, 1.0f, 0.65f);
            drawRing(0.0f, 0.0f, ringR, ringR - 0.008f, 28);
            glPopMatrix();

            // subtle connector line 
            glColor4f(0.12f, 0.7f, 1.0f, (leftPressed && rightPressed) ? 0.9f : 0.12f);
            drawQuad(leftX + 0.01f, ringY - 0.004f, rightX - 0.01f, ringY + 0.004f);

            if (leftPressed) glColor3f(0.06f, 0.85f, 1.0f); else glColor3f(0.55f, 0.55f, 0.55f);
            drawCircle(leftX, ringY, 0.01f, 12);
            if (rightPressed) glColor3f(0.06f, 0.85f, 1.0f); else glColor3f(0.55f, 0.55f, 0.55f);
            drawCircle(rightX, ringY, 0.01f, 12);

            // draw arrows above rings 
            drawArrowShape(leftX, arrowY, sequence[i].chordDirs[0], sequence[i].result, ARROW_DISPLAY_SIZE * 0.9f, animScale, sequence[i].isFake);
            drawArrowShape(rightX, arrowY, sequence[i].chordDirs[1], sequence[i].result, ARROW_DISPLAY_SIZE * 0.9f, animScale, sequence[i].isFake);
        }
        else
        {
            drawArrowShape(px, arrowY, sequence[i].dir, sequence[i].result, ARROW_DISPLAY_SIZE, animScale, sequence[i].isFake);
        }

        // draw hold indicator if this arrow is a hold note
        if (sequence[i].isHold)
        {
            float barW = 0.06f;
            float barH = 0.01f;
            float barX = px - barW / 2.0f;
            float barY = arrowY - 0.045f;

            // background
            glColor3f(0.2f, 0.2f, 0.2f);
            drawQuad(barX, barY - barH/2, barX + barW, barY + barH/2);

            // progress if holding
            if (i == seqIdx && sequence[i].isHolding && sequence[i].holdDurationMs > 0)
            {
                int now = glutGet(GLUT_ELAPSED_TIME);
                float progress = (float)(now - sequence[i].holdStartTimeMs) / (float)sequence[i].holdDurationMs;
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;

                glColor3f(0.1f, 0.7f, 1.0f);
                drawQuad(barX, barY - barH/2, barX + barW * progress, barY + barH/2);
            }
            else
            {
                // idle small marker to show its a hold note
                glColor3f(0.6f, 0.6f, 0.6f);
                drawQuad(barX + barW*0.5f - 0.005f, barY - barH/2, barX + barW*0.5f + 0.005f, barY + barH/2);
            }
        }
    }

    if (stepWaitingForInput && seqIdx < (int)sequence.size())
    {
        float barY = cy + 0.04f;
        float barW = w * 0.9f;
        float barX = cx - barW / 2;
        float barH = 0.01f;

        int currentPerfectMs = currentSequenceIsFast ? (TIMING_PERFECT_MS / 2) : TIMING_PERFECT_MS;
        int currentGoodMs = currentSequenceIsFast ? (TIMING_GOOD_MS / 2) : TIMING_GOOD_MS;
        int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

        float perfectP = (float)currentPerfectMs / currentBadMs;
        float goodP = (float)currentGoodMs / currentBadMs;

        glColor3f(0.1f, 1.0f, 0.2f); 
        drawQuad(barX, barY - barH, barX + barW * perfectP, barY + barH);
        glColor3f(0.4f, 0.8f, 0.2f); 
        drawQuad(barX + barW * perfectP, barY - barH, barX + barW * goodP, barY + barH);
        glColor3f(1.0f, 0.7f, 0.2f); 
        drawQuad(barX + barW * goodP, barY - barH, barX + barW, barY + barH);

        if (sequence[seqIdx].isFake)
        {
            glColor3f(1.0f, 0.2f, 0.2f);
            drawQuad(barX, barY - barH, barX + barW, barY + barH);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawString("WAIT!", cx - 0.05f, barY + 0.02f, FONT_SCALE_LARGE);
        }

        int dt = glutGet(GLUT_ELAPSED_TIME) - stepStartTimeMs;
        float progress = (float)dt / (float)currentBadMs;
        if (progress > 1.0f) progress = 1.0f;

        glColor3f(1.0f, 0.2f, 0.2f);
        float timerX = barX + barW * progress;
        drawQuad(timerX - 0.005f, barY - barH * 2.0f, timerX + 0.005f, barY + barH * 2.0f);
    }
}

void drawPlayerTrail()
{
    if (player.trail.size() < 2) return;

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(3.0f);

    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < player.trail.size(); ++i)
    {
        float progress = (float)i / (float)(player.trail.size() - 1);
        float alpha = progress * 0.6f;
        
        glColor4f(0.9f - (0.5f * progress), 0.2f + (0.6f * progress), 0.9f, alpha);
        glVertex2f(player.trail[i].x, player.trail[i].y);
    }
    glEnd();
    
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);
}

void updateShockwaves(float dt)
{
    for (size_t i = 0; i < shockwaves.size(); )
    {
        shockwaves[i].radius += 1.5f * dt; 
        shockwaves[i].alpha -= 1.5f * dt;  

        if (shockwaves[i].alpha <= 0.0f) {
            shockwaves[i] = shockwaves.back();
            shockwaves.pop_back();
        } else {
            i++;
        }
    }
}

void drawShockwaves()
{
    glLineWidth(3.0f);
    for (const auto& s : shockwaves)
    {
        glColor4f(0.4f, 1.0f, 1.0f, s.alpha); 
        drawRing(s.x, s.y, s.radius, s.radius - 0.02f, 32);
    }
}

void drawWarpEffect()
{
    if (!isLevelTransitioning) return;
    
    float progress = 1.0f - (levelTransitionTimer / WARP_DURATION); 
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int numLines = 40;
    float centerX = 0.0f;
    float centerY = 0.0f;
    
    glLineWidth(2.0f + progress * 5.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < numLines; ++i)
    {
        float angle = (float)i / numLines * 2.0f * 3.14159f;
        angle += glutGet(GLUT_ELAPSED_TIME) * 0.005f;
        
        float cosA = cosf(angle);
        float sinA = sinf(angle);
        
        float r1 = 0.1f + progress * 0.5f; 
        float r2 = 2.0f; 
        
        glColor4f(0.8f, 1.0f, 1.0f, progress); 
        glVertex2f(centerX + cosA * r1, centerY + sinA * r1);
        
        glColor4f(0.0f, 1.0f, 1.0f, 0.0f); 
        glVertex2f(centerX + cosA * r2, centerY + sinA * r2);
    }
    glEnd();
    
    if (progress > 0.9f)
    {
        float flashAlpha = (progress - 0.9f) * 10.0f; 
        glColor4f(1.0f, 1.0f, 1.0f, flashAlpha);
        drawQuad(-1, -1, 1, 1);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// HUD
void displayHUD()
{
    float hudH = 0.15f;
    float topY = 1.0f - hudH/2;
    
    glColor4f(0.05f, 0.05f, 0.07f, 0.9f); 
    drawQuad(-screenAspect, 1.0f - hudH, screenAspect, 1.0f);
    
    glColor4f(0.2f, 0.5f, 0.8f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(-screenAspect, 1.0f - hudH);
    glVertex2f(screenAspect, 1.0f - hudH);
    glEnd();

    float yC = topY + 0.01f; 

    drawKeyButton(-screenAspect + 0.15f, yC, 0.15f, 0.08f, "ESC", false);
    glColor3f(0.7f, 0.7f, 0.7f);
    drawString("MENU", -screenAspect + 0.25f, yC - 0.01f, FONT_SCALE_MEDIUM);

    float moveX = -0.6f; 
    float keySz = 0.07f;
    float gap = 0.01f;
    
    drawKeyButton(moveX, yC + keySz/2 + gap, keySz, keySz, "W", false);
    drawKeyButton(moveX - keySz - gap, yC - keySz/2, keySz, keySz, "A", false);
    drawKeyButton(moveX, yC - keySz/2, keySz, keySz, "S", false);
    drawKeyButton(moveX + keySz + gap, yC - keySz/2, keySz, keySz, "D", false);
    
    glColor3f(0.7f, 0.7f, 0.7f);
    drawString("SELECT", moveX - 0.05f, yC - 0.1f, FONT_SCALE_MEDIUM);

    float actionX = 0.2f;
    drawKeyButton(actionX, yC, 0.4f, 0.08f, "SPACE BAR", false);
    glColor3f(0.7f, 0.7f, 0.7f);
    drawString("ENGAGE", actionX - 0.06f, yC - 0.1f, FONT_SCALE_MEDIUM);

    float timerX = 0.0f; 
    float timerY = yC - 0.15f; 

    if (virus.active)
    {
        float pulse = sinf(glutGet(GLUT_ELAPSED_TIME) * 0.02f);
        if (pulse > 0) glColor3f(1.0f, 0.0f, 0.0f);
        else glColor3f(0.5f, 0.0f, 0.0f);

        std::string warning = "VIRUS ACTIVE! RUN!";
        drawString(warning.c_str(), timerX - 0.2f, timerY, FONT_SCALE_LARGE);
    }

    float statusX = screenAspect - 0.4f;
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string lvl = "LEVEL: " + std::to_string(currentLevel);
    drawString(lvl.c_str(), statusX, yC + 0.02f, FONT_SCALE_LARGE);
    
    drawString("ENERGY:", statusX, yC - 0.03f, FONT_SCALE_LARGE);
    float ex = statusX + 0.2f;
    float ey = yC - 0.02f;
    for(int i=0; i<playerLives; ++i) {
        drawEnergyIcon(ex + i*0.05f, ey, 0.015f, 1.0f);
    }

    if (gameMessageTimer > 0.0f)
    {
        float alpha = std::min(1.0f, gameMessageTimer / 0.5f);

        if (gameWon)
            glColor4f(0.2f, 1.0f, 0.3f, alpha);
        else if (isGameOver)
            glColor4f(1.0f, 0.2f, 0.2f, alpha);
        else
            glColor4f(1.0f, 0.9f, 0.2f, alpha);

        float msg_x = - (gameMessage.length() * FONT_SCALE_XLARGE * 3.7f) / 2.0f;
        drawString(gameMessage.c_str(), msg_x - 0.2f, -0.7f, FONT_SCALE_XLARGE);
    }
}

// MENUUUU
void drawMenu()
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    drawQuad(-screenAspect, -1.0f, screenAspect, 1.0f);

    float cx = -0.7f;
    float cy = 0.5f;
    float menuSpacing = 0.12f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("THISPATCHED", cx, cy, FONT_SCALE_XLARGE);
    cy -= menuSpacing * 1.5f;

    std::vector<std::string> menuItems = getMenuItems();
    if (menuSelectedItem >= (int)menuItems.size()) menuSelectedItem = 0;

    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        float y = cy - i * menuSpacing;

        if (i == menuSelectedItem)
        {
            glColor3f(0.1f, 1.0f, 0.2f);
            drawArrowShape(cx - 0.05f, y + 0.02f, ARROW_RIGHT, 0, 0.05f);
        }
        else
            glColor3f(1.0f, 1.0f, 1.0f);

        std::string itemText = menuItems[i];
        if (itemText == "Music Volume")
            itemText = std::string("Music Volume: < ") + std::to_string(musicVolume) + " >";
        else if (itemText == "SFX")
            itemText = std::string("SFX: < ") + (sfxEnabled ? "ON" : "OFF") + " >";

        drawString(itemText.c_str(), cx, y, FONT_SCALE_LARGE);
    }

    glColor3f(0.8f, 0.8f, 0.8f);
    drawString("Controls: W/S/Arrows to navigate, A/D/Arrows to change, SPACE/Enter to select", cx - 0.3f, cy - (menuItems.size() + 2) * menuSpacing, FONT_SCALE_MEDIUM);
}

// almanac / legend / codex
void drawAlmanac()
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    drawQuad(-screenAspect, -1.0f, screenAspect, 1.0f);

    float cx = 0.0f, cy = 0.0f, w = 1.6f, h = 1.8f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    glColor3f(0.9f, 0.2f, 0.2f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.05f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);
    drawString("CODEX // SYSTEM TOOLS // THISPATCHED", cx - 0.75f, cy + h / 2 - 0.035f, FONT_SCALE_LARGE);

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy - h / 2, cx + w / 2, cy - h / 2 + 0.03f);
    glColor3f(1, 1, 1);
    drawString("Press [ESC] or [SPACE] to close", cx - 0.3f, cy - h / 2 + 0.008f, FONT_SCALE_MEDIUM);

    float legendX = -0.7f;
    float legendY = 0.7f;
    float legendCol2X = 0.0f;
    float legendSpacing = 0.08f;
    float legendIconSize = 0.025f;
    float legendTextX = legendX + 0.06f;
    float legendCol2TextX = legendCol2X + 0.06f;
    float textYOff = -0.012f;

    drawAvatarShape(legendX + legendIconSize * 0.5f, legendY, legendIconSize * 0.9f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("You (The 'Patch')", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(0.95f, 0.95f, 0.95f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.6f);
    drawString("Unknown Node", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(0.22f, 0.45f, 0.85f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_VISITED * 0.6f);
    drawString("Cracked Node (Safe)", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(0.4f, 1.0f, 0.5f);
    drawRing(legendX + legendIconSize * 0.5f, legendY, NODE_RING_HIGHLIGHT_OUT * 0.5f, NODE_RING_HIGHLIGHT_IN * 0.5f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Target Node", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    drawNodeIcon(ICON_KEY, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Decrypter Key", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    drawNodeIcon(ICON_LOCK, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Encrypted Node (Firewall)", legendTextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    legendY = 0.7f;
    drawNodeIcon(ICON_REWARD, legendCol2X + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Energy Cell (+1 Life)", legendCol2TextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.3f, 0.3f);
    drawCircle(legendCol2X + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.6f);
    drawNodeIcon(ICON_HAZARD, legendCol2X + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Hazard Node (!)", legendCol2TextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.8f, 0.2f);
    drawCircle(legendCol2X + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.6f);
    drawNodeIcon(ICON_FAST, legendCol2X + 0.005f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Fast Node (Half-Time)", legendCol2TextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;

    glColor3f(0.8f, 0.3f, 1.0f);
    drawCircle(legendCol2X + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.6f);
    drawNodeIcon(ICON_WARP, legendCol2X + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Warp Node (Shortcut?)", legendCol2TextX, legendY + textYOff, FONT_SCALE_LARGE);
    legendY -= legendSpacing;
    
    {
        float hx = legendCol2X + legendIconSize * 0.5f;
        float hy = legendY - legendSpacing * 0.15f;
        float barW = 0.06f;
        float barH = 0.01f;
        glColor3f(0.1f, 0.6f, 1.0f);
        drawQuad(hx - barW/2, hy - barH/2, hx + barW/2, hy + barH/2);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawString("Hold Note (release 50%)", legendCol2TextX, hy - 0.012f, FONT_SCALE_LARGE);
        legendY -= legendSpacing;
    }

    {
        float cx = legendCol2X + legendIconSize * 0.5f;
        float cy2 = legendY - legendSpacing * 0.15f;
        float ringR = 0.01f;
        float span = 0.035f;
        float leftX = cx - span;
        float rightX = cx + span;
        glColor3f(0.12f, 0.7f, 1.0f);
        drawRing(leftX, cy2, ringR + 0.003f, ringR, 12);
        drawRing(rightX, cy2, ringR + 0.003f, ringR, 12);
        glColor4f(0.12f, 0.7f, 1.0f, 0.6f);
        drawQuad(leftX + 0.005f, cy2 - 0.002f, rightX - 0.005f, cy2 + 0.002f);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawString("Chord (simultaneous press)", legendCol2TextX, cy2 - 0.012f, FONT_SCALE_LARGE);
        legendY -= legendSpacing;
    }

    // spacer before the fake-note warning
    legendY -= legendSpacing * 0.5f;
    glColor3f(1.0f, 0.2f, 0.2f);
    drawArrowShape(legendCol2X + 0.1f, legendY, ARROW_LEFT, 0, 0.05f, 1.0f, true);
    drawArrowShape(legendCol2X + 0.18f, legendY, ARROW_RIGHT, 0, 0.05f, 1.0f, true);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("= FAKE! DO NOT PRESS!", legendCol2X + 0.25f, legendY - 0.01f, FONT_SCALE_LARGE);
}

void drawGame()
{
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // cam transform
    glPushMatrix();
    glTranslatef(-cameraX, -cameraY, 0);

    if (screenShakeTimer > 0.0f)
    {
        float shakeAmt = screenShakeTimer * 0.08f;
        float shakeX = (rand() % 100 / 50.0f - 1.0f) * shakeAmt;
        float shakeY = (rand() % 100 / 50.0f - 1.0f) * shakeAmt;
        glTranslatef(shakeX, shakeY, 0.0f);
    }

    drawMap();
    
    if (virus.active) {
        drawVirus(virus.x, virus.y);
    }
    
    drawShockwaves();

    // offscreen indicators
    if (!nodes.empty() && player.currentNode >= 0 && !inSequence && !player.moving && !displayingPassword)
    {
        clampHighlightedNeighbor();
        if (player.highlightedNeighbor >= 0)
        {
            int neighborIdx = nodes[player.currentNode].links[player.highlightedNeighbor];
            Node& target = nodes[neighborIdx];
            
            NodeIcon icon = ICON_NONE;
            if (target.type == NODE_REWARD) icon = ICON_REWARD;
            else if (target.type == NODE_HAZARD) icon = ICON_HAZARD;
            else if (target.type == NODE_FAST) icon = ICON_FAST;
            else if (target.type == NODE_WARP) icon = ICON_WARP;
            if (target.isKey) icon = ICON_KEY;
            else if (target.isLocked) icon = ICON_LOCK;
            if (target.isFinal) icon = ICON_NONE; 

            drawOffScreenIndicator(target.x, target.y, icon, target.type);
        }
    }
    
    if (virus.active) {
        // check if off screen, draw red warning
        drawOffScreenIndicator(virus.x, virus.y, ICON_NONE, NODE_HAZARD);
    }

    drawPlayerTrail();
    drawParticles();
    drawAvatar(player.x, player.y);
    
    // floating texts
    drawFloatingTexts();
    
    glPopMatrix(); // end cam transform

    // fixed hud
    displayHUD();
    drawMiniMap();

    // warping visual effect (teleportation between levels)
    drawWarpEffect(); 

    if (inSequence)
        drawSequenceUI();
    else if (displayingPassword)
        drawPasswordDisplay();

    // vignette effect
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float ratio = (float)windowW / (float)windowH;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f); 
    glVertex2f(0.0f, 0.0f);

    glColor4f(0.0f, 0.0f, 0.0f, 0.85f); 
    int segments = 42;
    float radius = 1.2f; 
    for (int i = 0; i <= segments; i++)
    {
        float theta = (float)i / segments * 2.0f * 3.14159f;
        float x = cosf(theta) * radius * (ratio > 1 ? ratio : 1.0f); 
        float y = sinf(theta) * radius * (ratio < 1 ? 1.0f/ratio : 1.0f);
        glVertex2f(x, y);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}