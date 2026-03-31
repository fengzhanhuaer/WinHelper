#include "matrix_rain.h"

#include <vector>
#include <cstdlib>
#include <ctime>

#include "imgui.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr float kCellSize = 14.0f; // pixel size of one character cell
static constexpr float kSpeedMin = 0.06f; // fall speed in rows/frame
static constexpr float kSpeedMax = 0.22f;

// Glyph pool
static const char* kGlyphPool[] = {
    "0","1","2","3","4","5","6","7","8","9",
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "!","@","#","$","%","^","&","*","<",">","?","/","\\","|",
};
static constexpr int kGlyphCount = (int)(sizeof(kGlyphPool) / sizeof(kGlyphPool[0]));

// ---------------------------------------------------------------------------
// Column state — each column has exactly ONE character falling, no trail.
// ---------------------------------------------------------------------------
struct RainColumn {
    float row;   // current fractional row of the single character
    float speed; // rows per frame
    char  glyph; // the character being displayed

    void reset(int rowCount) {
        // Spread initial positions over 3x screen height for temporal variety
        const int spread = rowCount > 0 ? rowCount * 3 : 120;
        row   = -(float)(std::rand() % spread);
        speed = kSpeedMin + (kSpeedMax - kSpeedMin) * (std::rand() / (float)RAND_MAX);
        glyph = kGlyphPool[std::rand() % kGlyphCount][0];
    }
};

static std::vector<RainColumn> g_rainCols;
static int  g_rainColCount = 0;
static bool g_rainInit     = false;

static void InitMatrixRain(int colCount, int rowCount) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    g_rainCols.resize(colCount);
    for (int c = 0; c < colCount; ++c)
        g_rainCols[c].reset(rowCount);
    g_rainColCount = colCount;
    g_rainInit     = true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void RenderMatrixBackground() {
    ImGuiIO&    io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float W  = io.DisplaySize.x;
    const float H  = io.DisplaySize.y;

    const int colCount = (int)(W / kCellSize);
    const int rowCount = (int)(H / kCellSize) + 1;

    if (!g_rainInit || colCount != g_rainColCount)
        InitMatrixRain(colCount, rowCount);

    for (int c = 0; c < colCount; ++c) {
        auto& col = g_rainCols[c];
        col.row += col.speed;

        // Reset once the character has fallen off the bottom
        if ((int)col.row >= rowCount) {
            col.reset(rowCount);
            col.row = 0.0f; // start fresh from the top next cycle
        }

        const int irow = (int)col.row;
        if (irow < 0) continue; // still above the screen

        // Randomly change the glyph each frame (flicker effect)
        if (std::rand() % 8 == 0)
            col.glyph = kGlyphPool[std::rand() % kGlyphCount][0];

        // Single bright-green character, no trail
        const ImVec4 colorf(0.3f, 1.0f, 0.3f, 0.92f);
        char buf[2] = { col.glyph, '\0' };
        dl->AddText(
            ImVec2(c * kCellSize, irow * kCellSize),
            ImGui::ColorConvertFloat4ToU32(colorf),
            buf);
    }
}
