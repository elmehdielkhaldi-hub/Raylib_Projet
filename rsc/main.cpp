#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "raylib.h"
#include <iostream>
#include <fstream>

class button {
private:
    Texture2D texture;
    Vector2 position;
    float scale;
    bool isHovered;

public:
    button(const char* imagepath, Vector2 imageposition, float scaleFactor) {
        Image image = LoadImage(imagepath);
        int originalWidth = image.width;
        int originalHeight = image.height;
        int newWidth = static_cast<int>(originalWidth * scaleFactor);
        int newHeight = static_cast<int>(originalHeight * scaleFactor);
        ImageResize(&image, newWidth, newHeight);
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        position = imageposition;
        scale = scaleFactor;
        isHovered = false;

        if (texture.id == 0) {
            std::cerr << "Error loading texture: " << imagepath << std::endl;
        }
    }

    ~button() {
        UnloadTexture(texture);
    }

    void draw() {
        Color tintColor = isHovered ? RED : WHITE;
        DrawTextureEx(texture, position, 0.0f, 1.0f, tintColor);
    }

    bool ispressed(Vector2 mousePos, bool mousePressed) {
        Rectangle rect = { position.x, position.y, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        return CheckCollisionPointRec(mousePos, rect) && mousePressed;
    }

    void updateHoverState(Vector2 mousePos) {
        Rectangle rect = { position.x, position.y, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        isHovered = CheckCollisionPointRec(mousePos, rect);
    }
};

class Maze {
private:
    std::vector<std::vector<int>> maze;
    int width;
    int height;
    int cellSize;

    void initializeMaze() {
        // Initialiser avec des murs partout
        maze = std::vector<std::vector<int>>(width, std::vector<int>(height, 1));
    }

   void generateMaze(int seed = 0) {
    if (seed != 0) {
        std::srand(seed);
    } else {
        std::srand(std::time(0));
    }

    // Commencer à partir d'une cellule impaire pour garantir des chemins
    int startX = 1;
    int startY = 1;
    maze[startX][startY] = 0;

    std::vector<std::pair<int, int>> stack;
    stack.push_back({startX, startY});

    while (!stack.empty()) {
        int x = stack.back().first;
        int y = stack.back().second;
        stack.pop_back();

        // Directions possibles (haut, droite, bas, gauche)
        std::vector<std::pair<int, int>> directions = {
            {0, -2}, {2, 0}, {0, 2}, {-2, 0}
        };

        // Mélanger les directions
        std::random_shuffle(directions.begin(), directions.end());

        for (const auto& dir : directions) {
            int nx = x + dir.first;
            int ny = y + dir.second;

            // Vérifier si la nouvelle position est valide et non visitée
            if (nx > 0 && nx < width - 1 && ny > 0 && ny < height - 1 && maze[nx][ny] == 1) {
                // Creuser un chemin
                maze[nx][ny] = 0;
                maze[x + dir.first / 2][y + dir.second / 2] = 0;
                stack.push_back({nx, ny});
            }
        }
    }

    // S'assurer que les entrées/sorties sont ouvertes
    maze[1][0] = 0;  // Entrée
    maze[width - 2][height - 1] = 0;  // Sortie

    // Vérifier si la sortie est bien accessible et créer un chemin si nécessaire
    bool pathExists = false;
    for (int x = 1; x < width - 1 && !pathExists; ++x) {
        for (int y = height - 2; y >= 0; --y) {
            if (maze[x][y] == 0) {
                maze[x][height - 2] = 0;
                pathExists = true;
                break;
            }
        }
    }

    // Si la sortie n'est toujours pas accessible, recréer un labyrinthe
    if (!pathExists) {
        generateMaze();  // Réessayer la génération du labyrinthe
    }
}

public:
    Maze() : width(0), height(0), cellSize(0) {}

    Maze(int w, int h, int size) : width(w), height(h), cellSize(size) {
        // Assurer des dimensions impaires pour une meilleure génération
        width = (w % 2 == 0) ? w + 1 : w;
        height = (h % 2 == 0) ? h + 1 : h;
        
        initializeMaze();
        generateMaze();
    }

    void draw(Texture2D wallTexture, Texture2D pathTexture) const {
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                if (maze[x][y] == 1) {
                    DrawTexture(wallTexture, x * cellSize, y * cellSize, WHITE);
                } else {
                    DrawTexture(pathTexture, x * cellSize, y * cellSize, WHITE);
                }
            }
        }
    }

    void drawGoal(Texture2D goalTexture) const {
        DrawTexture(goalTexture, (width - 2) * cellSize, (height - 2) * cellSize, WHITE);
    }

    bool isPath(int x, int y) const {
        // Vérification des limites avant l'accès
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return false;
        }
        return maze[x][y] == 0;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};


class Player {
private:
    int x, y;
    Texture2D texture;
    int cellSize;

    void adjustPosition() {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
    }

public:
    Player(int startX, int startY, Texture2D tex, int size) : x(startX), y(startY), texture(tex), cellSize(size) {}

    void draw() const {
        DrawTexture(texture, x * cellSize, y * cellSize, WHITE);
    }

    void move(const Maze& maze) {
        if (IsKeyPressed(KEY_UP) && maze.isPath(x, y - 1)) y--;
        if (IsKeyPressed(KEY_DOWN) && maze.isPath(x, y + 1)) y++;
        if (IsKeyPressed(KEY_LEFT) && maze.isPath(x - 1, y)) x--;
        if (IsKeyPressed(KEY_RIGHT) && maze.isPath(x + 1, y)) x++;
        adjustPosition();
    }

    bool hasWon(const Maze& maze) const {
        return x == maze.getWidth() - 2 && y == maze.getHeight() - 2;
    }

    void resetPosition() {
        x = 1;
        y = 1;
    }
};

// -------- Functions for Score Management --------
void saveScore(const std::string& playerName, double score, std::vector<std::pair<std::string, double>>& scores) {
    scores.push_back({playerName, score});
}

void loadScores(const std::vector<std::pair<std::string, double>>& scores) {
    int yOffset = 40;
    for (const auto& score : scores) {
        DrawText(TextFormat("%s: %.2f sec", score.first.c_str(), score.second), 20, yOffset, 20, DARKGRAY);
        yOffset += 30;
    }
}

// -------- Main Function --------
const int WIDTH_DEFAULT = 23;
const int HEIGHT_DEFAULT = 23;
const int CELL_SIZE_DEFAULT = 40;
const int RIGHT_PADDING = 200;

#define FFF6DD   (Color){ 255, 246, 221, 255 }   // Custom light cream color

int main() {
    int WIDTH = WIDTH_DEFAULT;
    int HEIGHT = HEIGHT_DEFAULT;
    int CELL_SIZE = CELL_SIZE_DEFAULT;

    InitWindow(WIDTH * CELL_SIZE + RIGHT_PADDING, HEIGHT * CELL_SIZE, "Maze Game with Chrono");
    InitAudioDevice();
    SetTargetFPS(60);

    // Charger les images pour les interfaces
    Texture2D interfaceImg = LoadTexture("img/interface_vrai.png");
    Texture2D interface2Img = LoadTexture("img/second_interface.png");

    // Charger les textures des boutons
    button playButton("img/play.png", {300, 749}, 0.25f);
    button quitButton("img/quit.png", {600, 749}, 0.25f);

    // Boutons pour la sélection du niveau
    button facileButton("img/easy.png", {410, 300}, 0.33f);
    button mediumButton("img/medium.png", {410, 450}, 0.33f);
    button hardButton("img/hard.png", {410, 600}, 0.33f);

    // Charger la musique
    Music music = LoadMusicStream("music/HateBit.mp3");
    PlayMusicStream(music);

    // Charger les textures du labyrinthe et du joueur
    Image wallImg = LoadImage("img/wall1.png");
    ImageResize(&wallImg, CELL_SIZE, CELL_SIZE);
    Texture2D wallTexture = LoadTextureFromImage(wallImg);
    UnloadImage(wallImg);

    Image pathImg = LoadImage("img/way.png");
    ImageResize(&pathImg, CELL_SIZE, CELL_SIZE);
    Texture2D pathTexture = LoadTextureFromImage(pathImg);
    UnloadImage(pathImg);

    Image playerImg = LoadImage("img/player.png");
    ImageResize(&playerImg, CELL_SIZE, CELL_SIZE);
    Texture2D playerTexture = LoadTextureFromImage(playerImg);
    UnloadImage(playerImg);

    Image goalImg = LoadImage("img/goal.png");
    ImageResize(&goalImg, CELL_SIZE, CELL_SIZE);
    Texture2D goalTexture = LoadTextureFromImage(goalImg);
    UnloadImage(goalImg);

    // Charger le bouton de redémarrage
     
    button restartButton("img/restart.png", {WIDTH * CELL_SIZE - 15, HEIGHT * CELL_SIZE / 20}, 0.15);
    button homeButton("img/home.png", {WIDTH * CELL_SIZE - 15, HEIGHT * CELL_SIZE / 10}, 0.15);

    Maze maze(WIDTH, HEIGHT, CELL_SIZE);
    Player player(1, 1, playerTexture, CELL_SIZE);

    bool gameStarted = false;
    bool gameEnded = false;
    bool inMenu = true; // Boolean pour savoir si on est dans le menu principal
    bool inLevelSelection = false; // Boolean pour savoir si on est dans l'interface de sélection du niveau
    bool showMainMenu = false; // Fix: Added this flag

    double startTime = 0.0;
    double endTime = 0.0;
    std::string playerName = "Player 1";

    // Stocker les scores
    std::vector<std::pair<std::string, double>> scores;

    while (!WindowShouldClose()) {

        UpdateMusicStream(music);

        if (IsKeyPressed(KEY_P)) PauseMusicStream(music);
        if (IsKeyPressed(KEY_R)) ResumeMusicStream(music);

        Vector2 mousePos = GetMousePosition();
        bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        // Si on est dans le menu principal
        if (inMenu) {
            // Si le bouton "Jouer" est pressé
            playButton.updateHoverState(mousePos);
            if (playButton.ispressed(mousePos, mousePressed)) {
                inMenu = false; // Quitter le menu principal
                inLevelSelection = true; // Passer à l'interface de sélection du niveau
            }

            // Si le bouton "Quitter" est pressé
            quitButton.updateHoverState(mousePos);
            if (quitButton.ispressed(mousePos, mousePressed)) {
                CloseWindow(); // Fermer le jeu
                return 0;
            }

            // Dessiner le menu principal
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(interfaceImg, 0, 0, WHITE);
            playButton.draw();
            quitButton.draw();
            EndDrawing();
        }
        // Si on est dans l'interface de sélection du niveau
        else if (inLevelSelection) {
            // Gérer les clics sur les boutons de sélection du niveau
            facileButton.updateHoverState(mousePos);
            // Pour le niveau facile
if (facileButton.ispressed(mousePos, mousePressed)) {
    WIDTH = 11; HEIGHT = 11; CELL_SIZE = 40; 
    maze = Maze(WIDTH, HEIGHT, CELL_SIZE);
    player = Player(1, 1, playerTexture, CELL_SIZE);
    gameStarted = true;
    startTime = GetTime();
    inLevelSelection = false;
}

// Pour le niveau moyen
mediumButton.updateHoverState(mousePos);
if (mediumButton.ispressed(mousePos, mousePressed)) {
    WIDTH = 21; HEIGHT = 21; CELL_SIZE = 40; 
    maze = Maze(WIDTH, HEIGHT, CELL_SIZE);
    player = Player(1, 1, playerTexture, CELL_SIZE);
    gameStarted = true;
    startTime = GetTime();
    inLevelSelection = false;
}
            hardButton.updateHoverState(mousePos);
            if (hardButton.ispressed(mousePos, mousePressed)) {
                WIDTH = 23; HEIGHT = 23; CELL_SIZE = 40; // Grand labyrinthe pour Difficile
                maze = Maze(WIDTH, HEIGHT, CELL_SIZE);
                player.resetPosition();
                gameStarted = true;
                startTime = 0.0;
                startTime = GetTime();
                inLevelSelection = false;
            }

            // Dessiner l'interface de sélection du niveau
            BeginDrawing();
            ClearBackground(FFF6DD);
            DrawTexture(interface2Img, 0, 0, WHITE);
            facileButton.draw();
            mediumButton.draw();
            hardButton.draw();
            EndDrawing();
        }
        else {
            // Gérer le jeu une fois le niveau choisi
            if (gameStarted && player.hasWon(maze)) {
                endTime = GetTime();
                gameEnded = true;
                gameStarted = false;
                saveScore(playerName, endTime - startTime, scores); // Sauvegarder le score
            }

            // Restart button functionality
    restartButton.updateHoverState(mousePos);
    if (restartButton.ispressed(mousePos, mousePressed)) {
        maze = Maze(WIDTH, HEIGHT, CELL_SIZE);  // Regenerate the maze
        player.resetPosition();                // Reset the player position
        startTime = GetTime();                 // Reset the game timer

        gameEnded = false;                     // Reset game state
        gameStarted = true;                    // Start the game again
    }

    homeButton.updateHoverState(mousePos);
if (homeButton.ispressed(mousePos, mousePressed)) {
    gameStarted = false;    // Stop the current game
    gameEnded = false;      // Reset the game end state
    inMenu = true;
    showMainMenu = true;    // Set a flag to show the main menu
}

            BeginDrawing();
            ClearBackground(FFF6DD);

            // Écran de fin de jeu
if (gameEnded) {
    ClearBackground(FFF6DD);
    DrawText("You Win!", GetScreenWidth() / 2 - 50, GetScreenHeight() / 2 - 60, 40, GREEN);
    DrawText(TextFormat("Final Time: %.2f seconds", endTime - startTime), GetScreenWidth() / 2 - 100, GetScreenHeight() / 2, 20, DARKGRAY);
    DrawText("Press ENTER to restart", GetScreenWidth() / 2 - 100, GetScreenHeight() / 2 + 40, 20, DARKGRAY);

    // Si ENTER est pressé, réinitialiser le jeu
    if (IsKeyPressed(KEY_ENTER)) {
        gameStarted = true;  // Démarre un nouveau jeu
        gameEnded = false;   // Réinitialise l'état de fin de jeu
        startTime = GetTime(); // Redémarre le chronomètre
        maze = Maze(WIDTH, HEIGHT, CELL_SIZE); // Génère un nouveau labyrinthe
        player.resetPosition(); // Réinitialise la position du joueur
    }
}
else {
    // Dessiner les éléments du jeu
    maze.draw(wallTexture, pathTexture);
    maze.drawGoal(goalTexture);
    player.move(maze);
    player.draw();

    // Dessiner le chronomètre
    DrawRectangle(WIDTH * CELL_SIZE, 0, RIGHT_PADDING, HEIGHT * CELL_SIZE, FFF6DD);
    if (gameStarted) {
        DrawText(TextFormat("Time: %.2f sec", GetTime() - startTime), WIDTH * CELL_SIZE + 20, 20, 20, DARKGRAY);
    } else {
        DrawText("Press ENTER to start", WIDTH * CELL_SIZE + 5, 20, 15, BLACK);
    }

    // Dessiner le bouton de redémarrage
    restartButton.draw();
    homeButton.draw();
}


EndDrawing();
loadScores(scores);

        }
    }

    UnloadMusicStream(music);
    UnloadTexture(wallTexture);
    UnloadTexture(pathTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(goalTexture);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}