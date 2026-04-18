#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <array>

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7735.h"
#include "wifi_qr_bitmap.h"

namespace {

constexpr char kApSsid[] = "games";
constexpr char kApPassword[] = "gamesgames";
constexpr uint16_t kUdpPort = 10000;
constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kWebSocketPort = 81;
constexpr uint8_t kDnsPort = 53;

constexpr int kBoardWidth = 10;
constexpr int kBoardHeight = 20;
constexpr int kCellSize = 8;
constexpr int kScreenWidth = 80;
constexpr int kScreenHeight = 160;

constexpr int kLcdHost = SPI2_HOST;
constexpr int kLcdMosiPin = 3;
constexpr int kLcdClockPin = 5;
constexpr int kLcdCsPin = 4;
constexpr int kLcdDcPin = 2;
constexpr int kLcdResetPin = 1;
constexpr int kLcdBacklightPin = 38;
constexpr int kToggleButtonPin = 0;
constexpr uint8_t kBacklightOnLevel = LOW;
constexpr uint8_t kBacklightOffLevel = HIGH;
constexpr uint32_t kBacklightPwmFreq = 1000;
constexpr uint8_t kBacklightPwmBits = 8;
constexpr uint8_t kBacklightPwmChannel = 3;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint8_t kMaxLevel = 9;
constexpr uint8_t kGravityFrames[kMaxLevel + 1] = {48, 43, 38, 33, 28, 23, 18, 13, 8, 6};

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

constexpr uint16_t kEmptyColor = rgb565(0, 0, 0);
constexpr uint16_t kQrBackgroundColor = rgb565(252, 252, 252);
constexpr uint16_t kQrForegroundColor = rgb565(0, 0, 0);
constexpr int kQrLabelScale = 2;
constexpr uint16_t kTestColors[4] = {rgb565(255, 0, 0), rgb565(0, 255, 0), rgb565(0, 0, 255), kEmptyColor};

struct ColorScheme {
  uint16_t backgroundColor;
  uint16_t gridColor;
  uint16_t highlightColor;
  uint16_t pieceColors[8];
};

constexpr ColorScheme kColorSchemes[kMaxLevel + 1] = {
    {rgb565(7, 10, 26), rgb565(16, 20, 42), rgb565(255, 255, 255), {kEmptyColor, rgb565(0, 224, 255), rgb565(255, 222, 59), rgb565(58, 108, 255), rgb565(0, 232, 122), rgb565(255, 146, 38), rgb565(255, 63, 72), rgb565(203, 63, 255)}},
    {rgb565(20, 8, 30), rgb565(36, 18, 52), rgb565(255, 246, 255), {kEmptyColor, rgb565(77, 239, 255), rgb565(255, 235, 98), rgb565(92, 156, 255), rgb565(92, 250, 156), rgb565(255, 161, 63), rgb565(255, 81, 145), rgb565(211, 98, 255)}},
    {rgb565(4, 24, 24), rgb565(12, 44, 44), rgb565(242, 255, 250), {kEmptyColor, rgb565(61, 247, 230), rgb565(255, 210, 67), rgb565(73, 119, 255), rgb565(117, 248, 85), rgb565(255, 129, 43), rgb565(255, 74, 92), rgb565(168, 96, 255)}},
    {rgb565(24, 10, 12), rgb565(46, 18, 24), rgb565(255, 248, 240), {kEmptyColor, rgb565(61, 233, 255), rgb565(255, 202, 69), rgb565(83, 132, 255), rgb565(76, 230, 122), rgb565(255, 121, 32), rgb565(255, 68, 102), rgb565(215, 86, 255)}},
    {rgb565(8, 14, 8), rgb565(20, 36, 20), rgb565(246, 255, 246), {kEmptyColor, rgb565(0, 239, 255), rgb565(255, 223, 51), rgb565(73, 126, 255), rgb565(82, 255, 123), rgb565(255, 154, 56), rgb565(255, 72, 72), rgb565(182, 79, 255)}},
    {rgb565(18, 18, 6), rgb565(40, 40, 14), rgb565(255, 255, 235), {kEmptyColor, rgb565(51, 238, 255), rgb565(255, 231, 84), rgb565(86, 132, 255), rgb565(96, 248, 101), rgb565(255, 167, 58), rgb565(255, 86, 66), rgb565(210, 99, 255)}},
    {rgb565(6, 18, 30), rgb565(14, 34, 58), rgb565(245, 250, 255), {kEmptyColor, rgb565(74, 230, 255), rgb565(255, 221, 64), rgb565(92, 148, 255), rgb565(75, 238, 140), rgb565(255, 143, 58), rgb565(255, 71, 118), rgb565(197, 89, 255)}},
    {rgb565(28, 8, 24), rgb565(52, 18, 44), rgb565(255, 244, 255), {kEmptyColor, rgb565(83, 232, 255), rgb565(255, 215, 90), rgb565(102, 144, 255), rgb565(112, 243, 124), rgb565(255, 141, 75), rgb565(255, 92, 104), rgb565(220, 112, 255)}},
    {rgb565(8, 8, 8), rgb565(24, 24, 24), rgb565(255, 255, 255), {kEmptyColor, rgb565(71, 244, 255), rgb565(255, 224, 81), rgb565(102, 143, 255), rgb565(102, 255, 143), rgb565(255, 158, 73), rgb565(255, 86, 86), rgb565(208, 96, 255)}},
    {rgb565(16, 3, 3), rgb565(34, 10, 10), rgb565(255, 246, 246), {kEmptyColor, rgb565(76, 243, 255), rgb565(255, 226, 76), rgb565(96, 146, 255), rgb565(104, 250, 120), rgb565(255, 149, 63), rgb565(255, 78, 78), rgb565(214, 90, 255)}},
};

struct Point {
  int8_t x;
  int8_t y;
};

struct Piece {
  Point rotations[4][4];
  uint8_t rotationCount;
  uint8_t colorIndex;
};

enum class Command : uint8_t {
  None,
  Left,
  Right,
  Down,
  RotateCw,
  RotateCcw,
  HardDrop,
  Restart,
  ToggleInfo,
};

enum class ScreenMode : uint8_t {
  Qr,
  Game,
};

WiFiUDP udp;
DNSServer dnsServer;
WebServer webServer(kHttpPort);
WebSocketsServer webSocket(kWebSocketPort);
esp_lcd_panel_handle_t panelHandle = nullptr;
esp_lcd_panel_io_handle_t ioHandle = nullptr;
std::array<uint16_t, kScreenWidth * kScreenHeight> frameBuffer = {};

uint8_t board[kBoardWidth][kBoardHeight] = {};
uint32_t score = 0;
uint32_t linesCleared = 0;
uint8_t currentLevel = 0;
uint32_t highScore = 0;
uint8_t highLevel = 0;
bool started = false;
bool gameOver = false;
ScreenMode screenMode = ScreenMode::Qr;
Command queuedCommand = Command::None;

bool lastButtonReading = HIGH;
bool lastButtonStableState = HIGH;
uint32_t lastButtonChangeAt = 0;

Point currentPos {4, 1};
uint8_t currentRotation = 0;
Piece currentPiece;

uint32_t nextFallAt = 0;

const char kControllerPage[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover,user-scalable=no">
  <title>games</title>
  <style>
    @font-face { font-family:'Modern Tetris'; src:url(data:font/otf;base64,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AAAAAAADAA0EYAABAAAAAAAEABUEbQABAAAAAAAFAAsEggABAAAAAAAGAAwEjQABAAAAAAAHACsEmQABAAAAAAAIABYExAABAAAAAAAJAAkE2gABAAAAAAAKAHIE4wABAAAAAAALAEAFVQABAAAAAAAMADsFlQABAAAAAAANABwF0AABAAAAAAAOACsF7AABAAAAAAATACkGFwABAAAAAAEAAAwGQAADAAEECQAAADAGTAADAAEECQABABoGfAADAAEECQACAA4GlgADAAEECQADABoGpAADAAEECQAEACoGvgADAAEECQAFABYG6AADAAEECQAGABgG/gADAAEECQAHAFYHFgADAAEECQAIACwHbAADAAEECQAJABIHmAADAAEECQAKAOgHqgADAAEECQALAIAIkgADAAEECQAMAHYJEgADAAEECQANADgJiAADAAEECQAOAFYJwAADAAEECQATAFIKFgADAAEECQEAABgKaABDAG8AcAB5AHIAaQBnAGgAdAAgAE0AbwB0AGgAZQByAGYAYQBuACAAMgAwADEAOABNAG8AZABlAHIAbgAgAFQAZQB0AHIAaQBzAFIAZQBnAHUAbABhAHIATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcwBNAG8AZABlAHIAbgAgAFQAZQB0AHIAaQBzACAAUgBlAGcAdQBsAGEAcgBWAGUAcgBzAGkAbwBuACAAMQAuADAATQBvAGQAZQByAG4AVABlAHQAcgBpAHMARgBvAG4AdABTAHQAcgB1AGMAdAAgAGkAcwAgAGEAIAB0AHIAYQBkAGUAbQBhAHIAawAgAG8AZgAgAEYAbwBuAHQAUwB0AHIAdQBjAHQALgBjAG8AbQBoAHQAdABwAHMAOgAvAC8AZgBvAG4AdABzAHQAcgB1AGMAdAAuAGMAbwBtAE0AbwB0AGgAZQByAGYAYQBuIBwATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcyAdACAAdwBhAHMAIABiAHUAaQBsAHQAIAB3AGkAdABoACAARgBvAG4AdABTAHQAcgB1AGMAdAAKAEQAZQBzAGkAZwBuAGUAcgAgAGQAZQBzAGMAcgBpAHAAdABpAG8AbgA6ACAAPABwAD4AQQAgAGYAbwBuAHQAIABiAGEAcwBlAGQAIABvAG4AIAB0AGgAZQAgAHAAcgBlAHYAaQBvAHUAcwAgAGwAbwBnAG8AIABvAGYAIABUAGUAdAByAGkAcwAuADwALwBwAD4ACgBoAHQAdABwAHMAOgAvAC8AZgBvAG4AdABzAHQAcgB1AGMAdAAuAGMAbwBtAC8AZgBvAG4AdABzAHQAcgB1AGMAdABpAG8AbgBzAC8AcwBoAG8AdwAvADEANAA4ADUANQAxADUALwBtAG8AZABlAHIAbgAtAHQAZQB0AHIAaQBzAGgAdABwczovL2ZvbnRzdHJ1Y3QuY29tL2ZvbnRzdHJ1Y3RvcnMvc2hvdy8xNDcwMjMwL21vdGhlcmZhbkNyZWF0aXZlIENvbW1vbnMgQXR0cmlidXRpb25odHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9saWNlbnNlcy9ieS8zLjAvRml2ZSBiaWcgcXVhY2tpbmcgemVwaHlycyBqb2x0IG15IHdheCBiZWRBdzBBY0Z4bFd3PT0AQwBvAHAAeQByAGkAZwBoAHQAIABNAG8AdABoAGUAcgBmAGEAbgAgADIAMAAxADgATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcwBSAGUAZwB1AGwAYQByAE0AbwBkAGUAcgBuACAAVABlAHQAcgBpAHMATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcwAgAFIAZQBnAHUAbABhAHIAVgBlAHIAcwBpAG8AbgAgADEALgAwAE0AbwBkAGUAcgBuAFQAZQB0AHIAaQBzAEYAbwBuAHQAUwB0AHIAdQBjAHQAIABpAHMAIABhACAAdAByAGEAZABlAG0AYQByAGsAIABvAGYAIABGAG8AbgB0AFMAdAByAHUAYwB0AC4AYwBvAG0AaAB0AHQAcABzADoALwAvAGYAbwBuAHQAcwB0AHIAdQBjAHQALgBjAG8AbQBNAG8AdABoAGUAcgBmAGEAbiAcAE0AbwBkAGUAcgBuACAAVABlAHQAcgBpAHMgHQAgAHcAYQBzACAAYgB1AGkAbAB0ACAAdwBpAHQAaAAgAEYAbwBuAHQAUwB0AHIAdQBjAHQACgBEAGUAcwBpAGcAbgBlAHIAIABkAGUAcwBjAHIAaQBwAHQAaQBvAG4AOgAgADwAcAA+AEEAIABmAG8AbgB0ACAAYgBhAHMAZQBkACAAbwBuACAAdABoAGUAIABwAHIAZQB2AGkAbwB1AHMAIABsAG8AZwBvACAAbwBmACAAVABlAHQAcgBpAHMALgA8AC8AcAA+AAoAaAB0AHQAcABzADoALwAvAGYAbwBuAHQAcwB0AHIAdQBjAHQALgBjAG8AbQAvAGYAbwBuAHQAcwB0AHIAdQBjAHQAaQBvAG4AcwAvAHMAaABvAHcALwAxADQAOAA1ADUAMQA1AC8AbQBvAGQAZQByAG4ALQB0AGUAdAByAGkAcwBoAHQAdABwAHMAOgAvAC8AZgBvAG4AdABzAHQAcgB1AGMAdAAuAGMAbwBtAC8AZgBvAG4AdABzAHQAcgB1AGMAdABvAHIAcwAvAHMAaABvAHcALwAxADQANwAwADIAMwAwAC8AbQBvAHQAaABlAHIAZgBhAG4AQwByAGUAYQB0AGkAdgBlACAAQwBvAG0AbQBvAG4AcwAgAEEAdAB0AHIAaQBiAHUAdABpAG8AbgBoAHQAdABwADoALwAvAGMAcgBlAGEAdABpAHYAZQBjAG8AbQBtAG8AbgBzAC4AbwByAGcALwBsAGkAYwBlAG5zAGUAcwAvAGIAeQAvADMALgAwAC8ARgBpAHYAZQAgAGIAaQBnACAAcQB1AGEAYwBrAGkAbgBnACAAegBlAHAAaAB5AHIAcwAgAGoAbwBsAHQAIABtAHkAIAB3AGEAeAAgAGIAZQBkAEEAdwAwAEEAYwBGAHgAbABXAHcAPQA9Q29weXJpZ2h0IE1vdGhlcmZhbiAyMDE4TW9kZXJuIFRldHJpc1JlZ3VsYXJNb2Rlcm4gVGV0cmlzTW9kZXJuIFRldHJpcyBSZWd1bGFyVmVyc2lvbiAxLjBNb2Rlcm5UZXRyaXNGb250U3RydWN0IGlzIGEgdHJhZGVtYXJrIG9mIEZvbnRTdHJ1Y3QuY29taHR0cHM6Ly9mb250c3RydWN0LmNvbU1vdGhlcmZhbtJNb2Rlcm4gVGV0cmlz0yB3YXMgYnVpbHQgd2l0aCBGb250U3RydWN0RGVzaWduZXIgZGVzY3JpcHRpb246IDxwPkEgZm9udCBiYXNlZCBvbiB0aGUgcHJldmlvdXMgbG9nbyBvZiBUZXRyaXMuPC9wPmh0dHBzOi8vZm9udHN0cnVjdC5jb20vZm9udHN0cnVjdGlvbnMvc2hvdy8xNDg1NTE1L21vZGVybi10ZXRyaXNodHRwczovL2ZvbnRzdHJ1Y3QuY29tL2ZvbnRzdHJ1Y3RvcnMvc2hvdy8xNDcwMjMwL21vdGhlcmZhbkNyZWF0aXZlIENvbW1vbnMgQXR0cmlidXRpb25odHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9saWNlbnNlcy9ieS8zLjAvRml2ZSBiaWcgcXVhY2tpbmcgemVwaHlycyBqb2x0IG15IHdheCBiZWRBdzBBY0Z4bFd3PT0AQwBvAHAAeQByAGkAZwBoAHQAIABNAG8AdABoAGUAcgBmAGEAbgAgADIAMAAxADgATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcwBSAGUAZwB1AGwAYQByAE0AbwBkAGUAcgBuACAAVABlAHQAcgBpAHMATQBvAGQAZQByAG4AIABUAGUAdAByAGkAcwAgAFIAZQBnAHUAbABhAHIAVgBlAHIAcwBpAG8AbgAgADEALgAwAE0AbwBkAGUAcgBuAFQAZQB0AHIAaQBzAEYAbwBuAHQAUwB0AHIAdQBjAHQAIABpAHMAIABhACAAdAByAGEAZABlAG0AYQByAGsAIABvAGYAIABGAG8AbgB0AFMAdAByAHUAYwB0AC4AYwBvAG0AaAB0AHQAcABzADoALwAvAGYAbwBuAHQAcwB0AHIAdQBjAHQALgBjAG8AbQBNAG8AdABoAGUAcgBmAGEAbiAcAE0AbwBkAGUAcgBuACAAVABlAHQAcgBpAHMgHQAgAHcAYQBzACAAYgB1AGkAbAB0ACAAdwBpAHQAaAAgAEYAbwBuAHQAUwB0AHIAdQBjAHQACgBEAGUAcwBpAGcAbgBlAHIAIABkAGUAcwBjAHIAaQBwAHQAaQBvAG4AOgAgADwAcAA+AEEAIABmAG8AbgB0ACAAYgBhAHMAZQBkACAAbwBuACAAdABoAGUAIABwAHIAZQB2AGkAbwB1AHMAIABsAG8AZwBvACAAbwBmACAAVABlAHQAcgBpAHMALgA8AC8AcAA+AAoAaAB0AHQAcABzADoALwAvAGYAbwBuAHQAcwB0AHIAdQBjAHQALgBjAG8AbQAvAGYAbwBuAHQAcwB0AHIAdQBjAHQAaQBvAG4AcwAvAHMAaABvAHcALwAxADQAOAA1ADUAMQA1AC8AbQBvAGQAZQByAG4ALQB0AGUAdAByAGkAcwBoAHQAdABwczovL2ZvbnRzdHJ1Y3QuY29tL2ZvbnRzdHJ1Y3RvcnMvc2hvdy8xNDcwMjMwL21vdGhlcmZhbkNyZWF0aXZlIENvbW1vbnMgQXR0cmlidXRpb25odHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9saWNlbnNlcy9ieS8zLjAvRml2ZSBiaWcgcXVhY2tpbmcgemVwaHlycyBqb2x0IG15IHdheCBiZWRBdzBBY0Z4bFd3PT0AAAADAAAAAAAAAMgAZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAKAB4ALAAAAWxhdG4ACAABAAAAAP//AAEAAAABa2VybgAIAAAAAQAAAAEABAACAAAAAQgAAQB+AAQAAAAHABgAHgAwAEYAWABeAHAAAQAp/fEABAAm/tIAJ/78ACj+pQAr/ngABQAT/O0ALf10ADH/LAA1/ykAO/78AAQAJv54ACj9ZwAKf32ACv9xgAEAE/2cAAEAEP7SAEL+/ABD/qcARf78AAMAQP7SAEP+/ABF/vwAAAAAADMCagAAAAAAAAAAADAAAAAAAAAAAAABABoAMAAAAAAAAAACAA4ASgAAAAAAAAADABoAWAAAAAAAAAAEACoAcgAAAAAAAAAFABYAnAAAAAAAAAAGABgAsgAAAAAAAAAHAFYAygAAAAAAAAAIACwBIAAAAAAAAAAJABIBTAAAAAAAAAAKAOgBXgAAAAAAAAALAIACRgAAAAAAAAAMAHYCxgAAAAAAAAANADgDPAAAAAAAAAAOAFYDdAAAAAAAAAATAFIDygAAAAAAAAEAABgEHAABAAAAAAAAABgENAABAAAAAAABAA0ETAABAAAAAAACAAcEWQABAAAAAAADAA0EYAABAAAAAAAEABUEbQABAAAAAAAFAAsEggABAAAAAAAGAAwEjQABAAAAAAAHACsEmQABAAAAAAAIABYExAABAAAAAAAJAAkE2gABAAAAAAAKAHIE4wABAAAAAAALAEAFVQABAAAAAAAMADsFlQABAAAAAAANABwF0AABAAAAAAAOACsF7AABAAAAAAATACkGFwABAAAAAAEAAAwGQAADAAEECQAAADAGTAADAAEECQABABoGfAADAAEECQACAA4GlgADAAEECQADABoGpAADAAEECQAEACoGvgADAAEECQAFABYG6AADAAEECQAGABgG/gADAAEECQAHAFYHFgADAAEECQAIACwHbAADAAEECQAJABIHmAADAAEECQAKAOgHqgADAAEECQALAIAIkgADAAEECQAMAHYJEgADAAEECQANADgJiAADAAEECQAOAFYJwAADAAEECQATAFIKFgADAAEECQEAABgKaA==) format('opentype'); font-weight:400; font-style:normal; }
    :root { color-scheme: dark; --bg:#111; --panel:#1a1a1a; --key:#262626; --accent:#7dd3fc; --text:#f5f5f5; }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body { margin:0; font-family: system-ui, sans-serif; background: radial-gradient(circle at top, #222, #0b0b0b 60%); color:var(--text); min-height:100vh; display:flex; align-items:center; justify-content:center; user-select:none; -webkit-user-select:none; }
    .shell { width:min(100vw, 520px); padding:18px; display:grid; gap:22px; }
    .masthead { display:grid; gap:8px; justify-items:center; margin-bottom:4px; }
    .title { margin:0; display:flex; flex-wrap:wrap; justify-content:center; gap:10px; font-family:'Modern Tetris','Arial Black',Impact,Haettenschweiler,'Arial Narrow Bold',sans-serif; font-size:clamp(1.85rem, 6vw, 2.75rem); font-weight:900; line-height:0.92; text-transform:uppercase; letter-spacing:0.08em; }
    .title span { display:inline-flex; align-items:center; justify-content:center; padding:0.14em 0.18em 0.1em; color:#fff; text-shadow:0 3px 0 rgba(0,0,0,0.22); }
    .title .pocket { color:#9bd8ff; }
    .title .tetris { color:#ffd166; }
    .subtitle { margin:0; font-size:0.82rem; letter-spacing:0.22em; text-transform:uppercase; color:#b9c0d4; }
    .stats { background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.08); border-radius:18px; padding:12px 14px; display:flex; justify-content:space-between; align-items:flex-start; gap:6px; }
    .stat { flex:1 1 0; min-width:0; display:grid; gap:3px; justify-items:center; align-content:start; text-align:center; }
    .stat b { display:flex; align-items:flex-start; justify-content:center; gap:4px; font-size:0.82rem; white-space:nowrap; line-height:1.1; }
    .label { font-size:0.78rem; letter-spacing:0.04em; text-transform:uppercase; color:#bdbdbd; white-space:nowrap; line-height:1.1; }
    .pad { display:grid; grid-template-columns: 1fr; gap:20px; }
    .cluster { background:rgba(255,255,255,0.05); border-radius:22px; padding:16px; }
    .controls { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); grid-template-rows:92px 92px 84px 52px; gap:12px; }
    button { width:100%; min-width:0; border:none; border-radius:24px; background:var(--key); color:var(--text); font-size:1.12rem; font-weight:700; line-height:1.1; padding:0 18px; display:flex; align-items:center; justify-content:center; text-align:center; touch-action:manipulation; }
    .controls button { min-height:92px; }
    .arrow { padding:0; background:linear-gradient(180deg, #334155, #1f2937); box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08); }
    .arrow svg { width:34px; height:34px; display:block; fill:none; stroke:#f5f5f5; stroke-width:3.5; stroke-linecap:round; stroke-linejoin:round; pointer-events:none; }
    .action { font-size:1.2rem; }
    .rotate { font-size:1rem; }
    .primary { background:linear-gradient(180deg, #0ea5e9, #0369a1); }
    .warn { background:linear-gradient(180deg, #f97316, #c2410c); }
    .restart { min-height:34px !important; font-size:0.86rem; margin-top:14px; border-radius:18px; background:linear-gradient(180deg, #ef476f, #b51748); }
    .wide { grid-column:span 2; }
    .full { grid-column:1 / -1; }
    .empty { visibility:hidden; }
    .stacked { flex-direction:column; gap:6px; }
    .subtext { font-size:0.72rem; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.78); }
    .score-level { font-size:0.82rem; color:#b9ecff; letter-spacing:0.04em; text-transform:uppercase; line-height:1.1; }
    .score-record { font-size:0.82rem; color:#ffd166; letter-spacing:0.04em; text-transform:uppercase; line-height:1.1; }
    .footer { text-align:center; color:#a3a3a3; font-size:0.85rem; }
    @media (max-width: 460px) {
      .shell { width:100vw; }
      .stats { padding:10px 10px; gap:4px; }
      .stat { gap:3px; }
      .stat b { gap:3px; font-size:0.76rem; }
      .label, .score-level, .score-record { font-size:0.72rem; }
      .controls { grid-template-rows:84px 84px 78px 46px; }
      .arrow svg { width:30px; height:30px; }
      .restart { min-height:30px !important; margin-top:12px; }
    }
  </style>
</head>
<body>
  <div class="shell">
    <div class="masthead">
      <h1 class="title" aria-label="Pocket Tetris"><span class="pocket">Pocket</span><span class="tetris">Tetris</span></h1>
    </div>
    <div class="stats">
      <div class="stat"><span class="label">Score</span><b><span id="score">0</span><span class="score-level" id="level">L0</span></b></div>
      <div class="stat"><span class="label">High Score</span><b><span class="score-record" id="record">0</span><span class="score-record" id="recordLevel">L0</span></b></div>
      <div class="stat"><span class="label">Lines</span><b id="lines">0</b></div>
      <div class="stat"><span class="label">State</span><b id="state">Live</b></div>
    </div>
    <div class="pad">
      <div class="cluster controls">
        <button class="primary action rotate" data-press="Z">Rotate L</button>
        <button class="arrow" data-hold="U" aria-label="Up">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 5 5 12"/><path d="M12 5 19 12"/><path d="M12 5v14"/></svg>
        </button>
        <button class="primary action rotate" data-press="X">Rotate R</button>
        <button class="arrow" data-hold="L" aria-label="Left">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M5 12 12 5"/><path d="M5 12 12 19"/><path d="M5 12h14"/></svg>
        </button>
        <button class="arrow" data-press="D" aria-label="Down">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 19 5 12"/><path d="M12 19 19 12"/><path d="M12 19V5"/></svg>
        </button>
        <button class="arrow" data-hold="R" aria-label="Right">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19 12 12 5"/><path d="M19 12 12 19"/><path d="M19 12H5"/></svg>
        </button>
        <button class="warn action full" data-press="U">Hard Drop</button>
        <button class="restart full" data-press="S">Restart</button>
      </div>
    </div>
    <div class="footer">Join games / gamesgames. Controls use WebSocket for low-latency browser input.</div>
  </div>
  <script>
    const scoreEl = document.getElementById('score');
    const levelEl = document.getElementById('level');
    const recordEl = document.getElementById('record');
    const recordLevelEl = document.getElementById('recordLevel');
    const linesEl = document.getElementById('lines');
    const stateEl = document.getElementById('state');
    let socket;
    let holdTimer = null;

    function connect() {
      socket = new WebSocket(`ws://${location.hostname}:81/`);
      socket.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          scoreEl.textContent = data.score ?? '0';
          levelEl.textContent = `L${data.level ?? 0}`;
          recordEl.textContent = data.highScore ?? '0';
          recordLevelEl.textContent = `L${data.highLevel ?? 0}`;
          linesEl.textContent = data.lines ?? '0';
          stateEl.textContent = !data.started ? 'QR' : (data.gameOver ? 'Game Over' : 'Live');
        } catch (_) {}
      };
      socket.onclose = () => setTimeout(connect, 1000);
    }

    function send(command) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
      }
    }

    function attachHold(button, command) {
      const start = (event) => {
        event.preventDefault();
        send(command);
        holdTimer = setInterval(() => send(command), 90);
      };
      const stop = () => {
        clearInterval(holdTimer);
        holdTimer = null;
      };
      button.addEventListener('pointerdown', start);
      button.addEventListener('pointerup', stop);
      button.addEventListener('pointerleave', stop);
      button.addEventListener('pointercancel', stop);
    }

    document.querySelectorAll('[data-press]').forEach((button) => {
      button.addEventListener('pointerdown', (event) => {
        event.preventDefault();
        send(button.dataset.press);
      });
    });

    document.querySelectorAll('[data-hold]').forEach((button) => attachHold(button, button.dataset.hold));
    connect();
  </script>
</body>
</html>
)HTML";

const Piece kPieces[7] = {
  {{{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 1},
  {{{{0, -1}, {1, -1}, {0, 0}, {1, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 1, 2},
  {{{{-1, -1}, {-1, 0}, {0, 0}, {1, 0}}, {{-1, 1}, {0, 1}, {0, 0}, {0, -1}}, {{-1, 0}, {0, 0}, {1, 0}, {1, 1}}, {{1, -1}, {0, -1}, {0, 0}, {0, 1}}}, 4, 3},
  {{{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}, {{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 4},
  {{{{-1, 0}, {0, 0}, {1, 0}, {1, -1}}, {{-1, -1}, {0, -1}, {0, 0}, {0, 1}}, {{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}}, 4, 5},
  {{{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 6},
  {{{{-1, 0}, {0, 0}, {1, 0}, {0, -1}}, {{0, -1}, {0, 0}, {0, 1}, {-1, 0}}, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}, {{0, -1}, {0, 0}, {0, 1}, {1, 0}}}, 4, 7},
};

uint32_t gravityIntervalMs() {
  return (static_cast<uint32_t>(kGravityFrames[currentLevel]) * 1000UL + 30UL) / 60UL;
}

const ColorScheme &currentScheme() {
  return kColorSchemes[currentLevel];
}

void setDisplayPower(bool enabled) {
  digitalWrite(kLcdBacklightPin, enabled ? kBacklightOnLevel : kBacklightOffLevel);
}

void initBacklight() {
  pinMode(kLcdBacklightPin, OUTPUT);
  digitalWrite(kLcdBacklightPin, kBacklightOffLevel);
  delay(20);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  ledcAttach(kLcdBacklightPin, kBacklightPwmFreq, kBacklightPwmBits);
#else
  ledcSetup(kBacklightPwmChannel, kBacklightPwmFreq, kBacklightPwmBits);
  ledcAttachPin(kLcdBacklightPin, kBacklightPwmChannel);
#endif

  ledcWrite(kBacklightPwmChannel, 0);
}

void clearBoard() {
  memset(board, 0, sizeof(board));
  score = 0;
  linesCleared = 0;
  currentLevel = 0;
  started = false;
  gameOver = false;
}

void updateLevel() {
  currentLevel = min<uint8_t>(kMaxLevel, static_cast<uint8_t>(linesCleared / 10));
}

void updateHighWaterMarks() {
  if (score > highScore) {
    highScore = score;
  }
  if (currentLevel > highLevel) {
    highLevel = currentLevel;
  }
}

void initToggleButton() {
  pinMode(kToggleButtonPin, INPUT_PULLUP);
  lastButtonReading = digitalRead(kToggleButtonPin);
  lastButtonStableState = lastButtonReading;
  lastButtonChangeAt = millis();
}

bool pollToggleButtonPressed() {
  const bool reading = digitalRead(kToggleButtonPin);
  const uint32_t now = millis();

  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeAt = now;
  }

  if ((now - lastButtonChangeAt) < kButtonDebounceMs || reading == lastButtonStableState) {
    return false;
  }

  lastButtonStableState = reading;
  return lastButtonStableState == LOW;
}

bool cellOccupiedByPiece(int x, int y, Point pos, uint8_t rotation, const Piece &piece) {
  for (uint8_t index = 0; index < 4; ++index) {
    const Point square = piece.rotations[rotation][index];
    if (pos.x + square.x == x && pos.y + square.y == y) {
      return true;
    }
  }
  return false;
}

bool canPlace(Point pos, uint8_t rotation, const Piece &piece) {
  for (uint8_t index = 0; index < 4; ++index) {
    const Point square = piece.rotations[rotation][index];
    const int x = pos.x + square.x;
    const int y = pos.y + square.y;
    if (x < 0 || x >= kBoardWidth || y < 0 || y >= kBoardHeight) {
      return false;
    }
    if (board[x][y] != 0) {
      return false;
    }
  }
  return true;
}

void fillFrame(uint16_t color) {
  frameBuffer.fill(color);
}

void flushFrame() {
  esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, kScreenWidth, kScreenHeight, frameBuffer.data());
}

const uint8_t *glyphForCharacter(char character) {
  static constexpr uint8_t kGlyphC[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
  static constexpr uint8_t kGlyphE[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
  static constexpr uint8_t kGlyphI[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
  static constexpr uint8_t kGlyphK[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
  static constexpr uint8_t kGlyphO[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static constexpr uint8_t kGlyphP[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
  static constexpr uint8_t kGlyphR[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static constexpr uint8_t kGlyphS[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
  static constexpr uint8_t kGlyphT[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

  switch (character) {
    case 'c':
      return kGlyphC;
    case 'e':
      return kGlyphE;
    case 'i':
      return kGlyphI;
    case 'k':
      return kGlyphK;
    case 'o':
      return kGlyphO;
    case 'p':
      return kGlyphP;
    case 'r':
      return kGlyphR;
    case 's':
      return kGlyphS;
    case 't':
      return kGlyphT;
    default:
      return nullptr;
  }
}

void drawFilledRect(int x, int y, int width, int height, uint16_t color) {
  for (int py = max(0, y); py < min(kScreenHeight, y + height); ++py) {
    for (int px = max(0, x); px < min(kScreenWidth, x + width); ++px) {
      frameBuffer[py * kScreenWidth + px] = color;
    }
  }
}

void drawQrLabel(const char *text, int y, uint16_t color) {
  constexpr int kGlyphWidth = 5;
  constexpr int kGlyphHeight = 7;
  constexpr int kGlyphSpacing = 1;

  int length = 0;
  while (text[length] != '\0') {
    ++length;
  }

  const int textWidth = ((length * kGlyphWidth) + ((length - 1) * kGlyphSpacing)) * kQrLabelScale;
  int cursorX = (kScreenWidth - textWidth) / 2;

  for (int index = 0; index < length; ++index) {
    const uint8_t *glyph = glyphForCharacter(text[index]);
    if (glyph != nullptr) {
      for (int row = 0; row < kGlyphHeight; ++row) {
        for (int column = 0; column < kGlyphWidth; ++column) {
          if ((glyph[row] & (1 << (kGlyphWidth - 1 - column))) != 0) {
            drawFilledRect(cursorX + (column * kQrLabelScale), y + (row * kQrLabelScale), kQrLabelScale, kQrLabelScale, color);
          }
        }
      }
    }
    cursorX += (kGlyphWidth + kGlyphSpacing) * kQrLabelScale;
  }
}

void drawQrScreen() {
  fillFrame(kQrBackgroundColor);

  const int topTextY = 21;
  const int originX = (kScreenWidth - WIFI_QR_WIDTH) / 2;
  const int originY = 43;
  const int bottomTextY = originY + WIFI_QR_HEIGHT + 8;

  drawQrLabel("pocket", topTextY, kQrForegroundColor);

  for (int y = 0; y < WIFI_QR_HEIGHT; ++y) {
    const uint8_t *row = &wifi_qr_bitmap[y * WIFI_QR_BPR];
    for (int x = 0; x < WIFI_QR_WIDTH; ++x) {
      const bool filled = (row[x / 8] & (0x80 >> (x % 8))) != 0;
      frameBuffer[(originY + y) * kScreenWidth + originX + x] = filled ? kQrForegroundColor : kQrBackgroundColor;
    }
  }

  drawQrLabel("tetris", bottomTextY, kQrForegroundColor);

  flushFrame();
}

void drawCell(int x, int y, uint16_t color, bool occupied) {
  const ColorScheme &scheme = currentScheme();
  const int px = x * kCellSize;
  const int py = y * kCellSize;
  for (int dy = 0; dy < kCellSize; ++dy) {
    for (int dx = 0; dx < kCellSize; ++dx) {
      const bool border = dx == 0 || dx == kCellSize - 1 || dy == 0 || dy == kCellSize - 1;
      uint16_t pixelColor = border ? scheme.gridColor : color;
      if (occupied && (dx == 1 || dy == 1)) {
        pixelColor = scheme.highlightColor;
      }
      frameBuffer[(py + dy) * kScreenWidth + (px + dx)] = pixelColor;
    }
  }
}

void drawBoard() {
  const ColorScheme &scheme = currentScheme();
  fillFrame(scheme.backgroundColor);
  for (int x = 0; x < kBoardWidth; ++x) {
    for (int y = 0; y < kBoardHeight; ++y) {
      uint16_t color = scheme.backgroundColor;
      bool occupied = false;
      if (board[x][y] != 0) {
        color = scheme.pieceColors[board[x][y]];
        occupied = true;
      }
      if (cellOccupiedByPiece(x, y, currentPos, currentRotation, currentPiece)) {
        color = scheme.pieceColors[currentPiece.colorIndex];
        occupied = true;
      }
      drawCell(x, y, color, occupied);
    }
  }
  flushFrame();
}

void broadcastStats() {
  char payload[176];
  snprintf(payload, sizeof(payload), "{\"score\":%lu,\"level\":%u,\"highScore\":%lu,\"highLevel\":%u,\"lines\":%lu,\"gameOver\":%s,\"started\":%s}",
           static_cast<unsigned long>(score),
           static_cast<unsigned>(currentLevel),
           static_cast<unsigned long>(highScore),
           static_cast<unsigned>(highLevel),
           static_cast<unsigned long>(linesCleared),
           gameOver ? "true" : "false",
           started ? "true" : "false");
  webSocket.broadcastTXT(payload);
}

void seedRandom() {
  const uint32_t seed = micros() ^ esp_random();
  randomSeed(seed);
}

bool spawnPiece() {
  currentPiece = kPieces[random(0, 7)];
  currentRotation = 0;
  currentPos = {4, 1};
  return canPlace(currentPos, currentRotation, currentPiece);
}

void mergeCurrentPiece() {
  for (uint8_t index = 0; index < 4; ++index) {
    const Point square = currentPiece.rotations[currentRotation][index];
    const int x = currentPos.x + square.x;
    const int y = currentPos.y + square.y;
    if (x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight) {
      board[x][y] = currentPiece.colorIndex;
    }
  }
}

void clearFullLines() {
  uint8_t clearedThisTurn = 0;

  for (int readRow = kBoardHeight - 1; readRow >= 0; --readRow) {
    bool full = true;
    for (int x = 0; x < kBoardWidth; ++x) {
      if (board[x][readRow] == 0) {
        full = false;
        break;
      }
    }

    if (!full) {
      continue;
    }

    ++clearedThisTurn;
    for (int shiftRow = readRow; shiftRow > 0; --shiftRow) {
      for (int x = 0; x < kBoardWidth; ++x) {
        board[x][shiftRow] = board[x][shiftRow - 1];
      }
    }
    for (int x = 0; x < kBoardWidth; ++x) {
      board[x][0] = 0;
    }
    ++readRow;
  }

  linesCleared += clearedThisTurn;
  updateLevel();
  updateHighWaterMarks();
  switch (clearedThisTurn) {
    case 1:
      score += 100;
      break;
    case 2:
      score += 300;
      break;
    case 3:
      score += 500;
      break;
    case 4:
      score += 800;
      break;
    default:
      break;
  }
  if (clearedThisTurn > 0) {
    broadcastStats();
  }
}

void lockCurrentPiece() {
  mergeCurrentPiece();
  score += 10;
  updateHighWaterMarks();
  clearFullLines();
  if (!spawnPiece()) {
    gameOver = true;
    drawBoard();
  }
  broadcastStats();
}

void showQrMode() {
  clearBoard();
  screenMode = ScreenMode::Qr;
  drawQrScreen();
  broadcastStats();
}

void startGame() {
  clearBoard();
  started = true;
  screenMode = ScreenMode::Game;
  gameOver = !spawnPiece();
  updateHighWaterMarks();
  nextFallAt = millis() + gravityIntervalMs();
  drawBoard();
  broadcastStats();
}

Command mapInput(int value) {
  switch (toupper(value)) {
    case '4':
    case 'L':
      return Command::Left;
    case '6':
    case 'R':
      return Command::Right;
    case '2':
    case 'D':
      return Command::Down;
    case 'X':
    case 'C':
      return Command::RotateCw;
    case 'Z':
    case 'A':
      return Command::RotateCcw;
    case '8':
    case 'U':
      return Command::HardDrop;
    case 'S':
      return Command::Restart;
    default:
      return Command::None;
  }
}

Command pollCommand() {
  Command command = queuedCommand;
  queuedCommand = Command::None;
  while (Serial.available() > 0) {
    command = mapInput(Serial.read());
  }
  const int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    while (udp.available() > 0) {
      command = mapInput(udp.read());
    }
  }
  if (pollToggleButtonPressed()) {
    command = Command::ToggleInfo;
  }
  return command;
}

void queueCommand(Command command) {
  if (command != Command::None) {
    queuedCommand = command;
  }
}

void sendPortalPage() {
  webServer.send_P(200, "text/html", kControllerPage);
}

void redirectToPortal() {
  webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  webServer.send(302, "text/plain", "");
}

void tryMove(Point nextPos, uint8_t nextRotation) {
  if (canPlace(nextPos, nextRotation, currentPiece)) {
    currentPos = nextPos;
    currentRotation = nextRotation;
    drawBoard();
    return;
  }

  if (nextPos.x == currentPos.x && nextPos.y == currentPos.y + 1 && nextRotation == currentRotation) {
    lockCurrentPiece();
    if (!gameOver) {
      nextFallAt = millis() + gravityIntervalMs();
      drawBoard();
    }
  }
}

void applyCommand(Command command) {
  if (command == Command::None) {
    return;
  }

  if (command == Command::ToggleInfo) {
    if (screenMode == ScreenMode::Qr) {
      startGame();
    } else {
      showQrMode();
    }
    return;
  }

  if (command == Command::Restart) {
    startGame();
    return;
  }

  if (screenMode == ScreenMode::Qr || gameOver) {
    return;
  }

  switch (command) {
    case Command::Left:
      tryMove(Point{static_cast<int8_t>(currentPos.x - 1), currentPos.y}, currentRotation);
      break;
    case Command::Right:
      tryMove(Point{static_cast<int8_t>(currentPos.x + 1), currentPos.y}, currentRotation);
      break;
    case Command::Down:
      tryMove(Point{currentPos.x, static_cast<int8_t>(currentPos.y + 1)}, currentRotation);
      nextFallAt = millis() + gravityIntervalMs();
      break;
    case Command::RotateCw:
      tryMove(currentPos, (currentRotation + 1) % currentPiece.rotationCount);
      break;
    case Command::RotateCcw:
      tryMove(currentPos, (currentRotation + currentPiece.rotationCount - 1) % currentPiece.rotationCount);
      break;
    case Command::HardDrop: {
      Point dropped = currentPos;
      while (canPlace(Point{dropped.x, static_cast<int8_t>(dropped.y + 1)}, currentRotation, currentPiece)) {
        ++dropped.y;
      }
      currentPos = dropped;
      lockCurrentPiece();
      if (!gameOver) {
        nextFallAt = millis() + gravityIntervalMs();
        drawBoard();
      }
      break;
    }
    case Command::Restart:
      break;
    case Command::ToggleInfo:
      break;
    case Command::None:
      break;
  }
}

void setupWifiUdp() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAP(kApSsid, kApPassword);
  udp.begin(kUdpPort);
  dnsServer.start(kDnsPort, "*", WiFi.softAPIP());

  webServer.on("/", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/generate_204", HTTP_GET, []() {
    redirectToPortal();
  });
  webServer.on("/gen_204", HTTP_GET, []() {
    redirectToPortal();
  });
  webServer.on("/hotspot-detect.html", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/library/test/success.html", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/connecttest.txt", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/ncsi.txt", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/success.txt", HTTP_GET, []() {
    sendPortalPage();
  });
  webServer.on("/fwlink", HTTP_GET, []() {
    redirectToPortal();
  });
  webServer.on("/redirect", HTTP_GET, []() {
    redirectToPortal();
  });
  webServer.onNotFound([]() {
    redirectToPortal();
  });
  webServer.begin();

  webSocket.begin();
  webSocket.onEvent([](uint8_t clientNum, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      broadcastStats();
      return;
    }
    if (type != WStype_TEXT || length == 0) {
      return;
    }
    queueCommand(mapInput(payload[0]));
  });

  Serial.println();
  Serial.print("AP SSID: ");
  Serial.println(kApSsid);
  Serial.print("AP PASS: ");
  Serial.println(kApPassword);
  Serial.print("UDP port: ");
  Serial.println(kUdpPort);
  Serial.print("WebSocket port: ");
  Serial.println(kWebSocketPort);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupDisplay() {
  initBacklight();

  static esp_lcd_panel_dev_config_t panelConfig = {
      .reset_gpio_num = kLcdResetPin,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
      .color_space = ESP_LCD_COLOR_SPACE_BGR,
#else
      .color_space = LCD_RGB_ELEMENT_ORDER_BGR,
      .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
#endif
      .bits_per_pixel = 16,
  };
  static spi_bus_config_t spiConfig = ST7735_PANEL_BUS_SPI_CONFIG(kLcdClockPin, kLcdMosiPin, kScreenWidth * kScreenHeight * sizeof(uint16_t));
  static esp_lcd_panel_io_spi_config_t ioConfig = ST7735_PANEL_IO_SPI_CONFIG(kLcdCsPin, kLcdDcPin, NULL, NULL);

  ESP_ERROR_CHECK(spi_bus_initialize(static_cast<spi_host_device_t>(kLcdHost), &spiConfig, SPI_DMA_CH_AUTO));
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)kLcdHost, &ioConfig, &ioHandle));
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(ioHandle, &panelConfig, &panelHandle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panelHandle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panelHandle, 26, 1));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panelHandle, false));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panelHandle, true, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));

  setDisplayPower(true);
  delay(30);

  for (uint16_t color : kTestColors) {
    fillFrame(color);
    flushFrame();
    delay(180);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Booting LilyGO Tetris");

  setupDisplay();
  seedRandom();
  initToggleButton();
  setupWifiUdp();
  showQrMode();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  webSocket.loop();

  const Command command = pollCommand();
  applyCommand(command);

  if (!started || gameOver) {
    delay(5);
    return;
  }

  const uint32_t now = millis();
  if (now >= nextFallAt) {
    tryMove(Point{currentPos.x, static_cast<int8_t>(currentPos.y + 1)}, currentRotation);
    nextFallAt = now + gravityIntervalMs();
  }

  delay(5);
}
