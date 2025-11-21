#include "game.h"
#include <algorithm>

void specialKeys(int key, int x, int y)
{
    if (currentGameState == STATE_MENU)
    {
        std::vector<std::string> menuItems = getMenuItems();
        int numItems = (int)menuItems.size();
        if (menuSelectedItem >= numItems) menuSelectedItem = 0;

        if (key == GLUT_KEY_UP)
        {
            menuSelectedItem = (menuSelectedItem - 1 + numItems) % numItems;
            playSFX(SFX_MENU_SELECT);
        }
        else if (key == GLUT_KEY_DOWN)
        {
            menuSelectedItem = (menuSelectedItem + 1) % numItems;
            playSFX(SFX_MENU_SELECT);
        }
        else if (key == GLUT_KEY_LEFT)
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume") {
                musicVolume = std::max(0, musicVolume - 10);
                updateMusicVolume();
                playSFX(SFX_MENU_SELECT);
            }
            else if (item == "SFX") {
                sfxEnabled = !sfxEnabled;
                playSFX(SFX_MENU_SELECT);
            }
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume") {
                musicVolume = std::min(100, musicVolume + 10);
                updateMusicVolume();
                playSFX(SFX_MENU_SELECT);
            }
            else if (item == "SFX") {
                sfxEnabled = !sfxEnabled;
                playSFX(SFX_MENU_SELECT);
            }
        }
    }
    else if (currentGameState == STATE_PLAYING)
    {
        if (displayingPassword || isGameOver || gameWon || isLevelTransitioning) return;

        ArrowDir pressed;
        if (inSequence && (stepWaitingForInput || isPasswordInput))
        {
            if (key == GLUT_KEY_UP) pressed = ARROW_UP;
            else if (key == GLUT_KEY_DOWN) pressed = ARROW_DOWN;
            else if (key == GLUT_KEY_LEFT) pressed = ARROW_LEFT;
            else if (key == GLUT_KEY_RIGHT) pressed = ARROW_RIGHT;
            else return;

            handleArrowPress(pressed);
        }
        else if (!inSequence && !player.moving)
        {
            if (key == GLUT_KEY_LEFT) {
                player.highlightedNeighbor--;
                clampHighlightedNeighbor();
                playSFX(SFX_MOVE_HIGHLIGHT);
            }
            if (key == GLUT_KEY_RIGHT) {
                player.highlightedNeighbor++;
                clampHighlightedNeighbor();
                playSFX(SFX_MOVE_HIGHLIGHT);
            }
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char k, int x, int y)
{
    if (k == 27) // ESC key
    {
        if (isGameOver) {
            currentGameState = STATE_MENU;
            setGameMessage("", 0.0f);
        }
        else if (displayingPassword || isLevelTransitioning) {
            return;
        }
        else if (currentGameState == STATE_ALMANAC)
        {
            currentGameState = STATE_MENU;
        }
        else if (currentGameState == STATE_PLAYING) {
            currentGameState = STATE_MENU;
            setGameMessage("Paused", 1.0f);
        }
        else if (currentGameState == STATE_MENU && gameInProgress) {
            currentGameState = STATE_PLAYING;
            setGameMessage("", 0.0f);
        }
        glutPostRedisplay();
        return;
    }

    if (displayingPassword)
    {
        if (k == ' ' || k == 13) {
            displayingPassword = false;
            if (justRevealedPassword) {
                Node& keyNode = nodes[player.targetNode];
                keyNode.visited = true;
                startMoveAnimation(player.currentNode, player.targetNode);
                player.currentNode = player.targetNode;
                player.targetNode = -1;
                player.highlightedNeighbor = -1;
            }
        }
        glutPostRedisplay();
        return;
    }

    if (currentGameState == STATE_ALMANAC)
    {
        if (k == ' ' || k == 13)
        {
            currentGameState = STATE_MENU;
        }
        glutPostRedisplay();
        return;
    }

    if (currentGameState == STATE_MENU)
    {
        std::vector<std::string> menuItems = getMenuItems();
        int numItems = (int)menuItems.size();
        if (menuSelectedItem >= numItems) menuSelectedItem = 0;

        if (k == 'w' || k == 'W') {
            menuSelectedItem = (menuSelectedItem - 1 + numItems) % numItems;
            playSFX(SFX_MENU_SELECT);
        }
        else if (k == 's' || k == 'S') {
            menuSelectedItem = (menuSelectedItem + 1) % numItems;
            playSFX(SFX_MENU_SELECT);
        }
       
        else if (k == 'a' || k == 'A')
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume") {
                musicVolume = std::max(0, musicVolume - 10);
                updateMusicVolume();
                playSFX(SFX_MENU_SELECT);
            }
            else if (item == "SFX") {
                sfxEnabled = !sfxEnabled;
                playSFX(SFX_MENU_SELECT);
            }
        }
        else if (k == 'd' || k == 'D')
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume") {
                musicVolume = std::min(100, musicVolume + 10);
                updateMusicVolume();
                playSFX(SFX_MENU_SELECT);
            }
            else if (item == "SFX") {
                sfxEnabled = !sfxEnabled;
                playSFX(SFX_MENU_SELECT);
            }
        }
        // SPACE/ENTER for Selection
        else if (k == ' ' || k == 13) {
            const std::string& item = menuItems[menuSelectedItem];
            playSFX(SFX_MENU_ENGAGE);
            if (item == "New Game" || item == "Restart Game") {
                resetGame();
                currentGameState = STATE_PLAYING;
            }
            else if (item == "Resume Game") {
                currentGameState = STATE_PLAYING;
                setGameMessage("", 0.0f);
            }
            else if (item == "Codex") {
                currentGameState = STATE_ALMANAC;
            }
            else if (item == "Quit") {
                exit(0);
            }
        }
        glutPostRedisplay();
        return;
    }

    if (currentGameState == STATE_PLAYING)
    {
        if (isGameOver) return; 

        if (gameWon)
        {
            if (k == ' ' || k == 13) {
                if (!isLevelTransitioning) {
                    isLevelTransitioning = true;
                    levelTransitionTimer = WARP_DURATION; 
                    playSFX(SFX_MENU_ENGAGE); 
                    setGameMessage("WARP SEQUENCE INITIATED...", WARP_DURATION);
                }
            }
            glutPostRedisplay();
            return;
        }

        if (isLevelTransitioning) return; // no input during transition /warp

        if (inSequence && (stepWaitingForInput || isPasswordInput))
        {
            if (isPasswordInput && (k == ' ' || k == 13)) {
                inSequence = false;
                isPasswordInput = false;
                sequence.clear();
                seqIdx = 0;
                player.targetNode = -1;
                stepWaitingForInput = false;
                currentSequenceIsHazard = false;
                hazardSequencesRemaining = 0;
                totalHazardSequences = 0;
                setGameMessage("Password Cancelled", 2.0f);
                glutPostRedisplay();
                return;
            }

            ArrowDir pressed;
            if (k == 'w' || k == 'W') pressed = ARROW_UP;
            else if (k == 's' || k == 'S') pressed = ARROW_DOWN;
            else if (k == 'a' || k == 'A') pressed = ARROW_LEFT;
            else if (k == 'd' || k == 'D') pressed = ARROW_RIGHT;
            else return;

            handleArrowPress(pressed);
            glutPostRedisplay();
            return;
        }

        if (k == ' ' || k == 13)
        {
            if (inSequence) {
                // cancel sequence
                inSequence = false;
                isPasswordInput = false;
                sequence.clear();
                seqIdx = 0;
                player.targetNode = -1;
                stepWaitingForInput = false;
                currentSequenceIsHazard = false;
                hazardSequencesRemaining = 0;
                totalHazardSequences = 0;
                setGameMessage("Sequence Cancelled", 2.0f);
            }
            else {
                if (player.highlightedNeighbor == -1) {
                    // inspect key
                    Node& selfNode = nodes[player.currentNode];
                    int lockId = -1;
                    if (selfNode.isKey) lockId = selfNode.lockNode;

                    if (lockId != -1 && revealedPasswords.count(lockId)) {
                        sequence.clear();
                        for (ArrowDir dir : revealedPasswords[lockId])
                            sequence.emplace_back(dir, 0, 1.0f, false);
                        displayingPassword = true;
                        justRevealedPassword = false;
                        setGameMessage("Viewing Password. Press SPACE.", 1000.0f);
                    }
                    else if (selfNode.isKey) {
                        setGameMessage("Password not yet discovered!", 2.5f);
                        playSFX(SFX_TIMING_FAIL); 
                    }
                    else {
                        playSFX(SFX_TIMING_FAIL); 
                    }
                }
                else {
                    attemptMoveToHighlightedNeighbor();
                }
            }
            glutPostRedisplay();
            return;
        }

        if (!inSequence && !player.moving)
        {
            if (k == 'a' || k == 'A') {
                player.highlightedNeighbor--;
                clampHighlightedNeighbor();
                playSFX(SFX_MOVE_HIGHLIGHT);
            }
            if (k == 'd' || k == 'D') {
                player.highlightedNeighbor++;
                clampHighlightedNeighbor();
                playSFX(SFX_MOVE_HIGHLIGHT);
            }
            glutPostRedisplay();
        }
    }
}