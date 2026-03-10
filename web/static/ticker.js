// Sports Score Ticker - LED Panel Simulator & Config Editor
//
// Replicates the C++ ticker rendering on an HTML5 Canvas.
// Display: 320x64 pixels, matching the physical 10x P4 panel layout.

(function () {
  "use strict";

  // --- Constants (match C++ ticker.cpp) ---
  const DISPLAY_W = 320;
  const DISPLAY_H = 64;
  const GAME_CARD_GAP = 10;
  const SPORT_DIVIDER_W = 20;
  const LOGO_SIZE = 28;

  // Color palette (match C++)
  const WHITE  = [255, 255, 255];
  const YELLOW = [255, 255,   0];
  const GREEN  = [  0, 255,   0];
  const RED    = [255,   0,   0];
  const CYAN   = [  0, 255, 255];
  const GRAY   = [128, 128, 128];
  const ORANGE = [255, 165,   0];
  const DIM    = [ 80,  80,  80];

  const SPORT_COLORS = {
    nba:   [ 29,  66, 138],
    nfl:   [  1,  51, 105],
    mlb:   [  0,  45,  98],
    nhl:   [  0,   0,   0],
    ncaaf: [128,   0,   0],
    ncaam: [  0, 100,   0],
  };

  const SPORT_LABELS = {
    nba: "NBA", nfl: "NFL", mlb: "MLB",
    nhl: "NHL", ncaaf: "CFB", ncaam: "CBB",
  };

  // --- Bitmap font (6x10) ---
  // Minimal bitmap font for ASCII 32-127, matching 6x10 BDF
  // Each character is 6 pixels wide, 10 pixels tall
  // Stored as array of 10 rows of 6-bit values per character

  const FONT_W = 6;
  const FONT_H = 10;
  const LARGE_FONT_W = 9;
  const LARGE_FONT_H = 15;

  // We'll use canvas-based text rendering scaled to pixel grid
  // This gives us a clean bitmap look while supporting full ASCII

  let fontCanvas, fontCtx;
  let largeFontCanvas, largeFontCtx;

  function initFontRenderers() {
    fontCanvas = document.createElement("canvas");
    fontCanvas.width = 512;
    fontCanvas.height = 16;
    fontCtx = fontCanvas.getContext("2d");
    fontCtx.imageSmoothingEnabled = false;

    largeFontCanvas = document.createElement("canvas");
    largeFontCanvas.width = 512;
    largeFontCanvas.height = 20;
    largeFontCtx = largeFontCanvas.getContext("2d");
    largeFontCtx.imageSmoothingEnabled = false;
  }

  // Render text to a small off-screen canvas to get bitmap data
  function getTextBitmap(text, large) {
    const ctx = large ? largeFontCtx : fontCtx;
    const cvs = large ? largeFontCanvas : fontCanvas;
    const h = large ? LARGE_FONT_H : FONT_H;
    const fontSize = large ? 13 : 9;

    ctx.clearRect(0, 0, cvs.width, cvs.height);
    ctx.fillStyle = "#ffffff";
    ctx.font = `bold ${fontSize}px monospace`;
    ctx.textBaseline = "top";
    ctx.fillText(text, 0, 1);

    const metrics = ctx.measureText(text);
    const w = Math.ceil(metrics.width);
    if (w <= 0) return { pixels: null, width: 0, height: h };

    const imageData = ctx.getImageData(0, 0, w, h);
    return { pixels: imageData.data, width: w, height: h };
  }

  // --- Pixel buffer ---
  // We render into a 320x64 pixel buffer, then blit to canvas at scale

  let pixelBuffer; // Uint8Array, DISPLAY_W * DISPLAY_H * 3 (RGB)

  function clearBuffer() {
    pixelBuffer.fill(0);
  }

  function setPixel(x, y, r, g, b) {
    if (x < 0 || x >= DISPLAY_W || y < 0 || y >= DISPLAY_H) return;
    const idx = (y * DISPLAY_W + x) * 3;
    pixelBuffer[idx]     = r;
    pixelBuffer[idx + 1] = g;
    pixelBuffer[idx + 2] = b;
  }

  function drawText(x, baselineY, color, text, large) {
    const bmp = getTextBitmap(text, large);
    if (!bmp.pixels) return x;
    const h = bmp.height;
    // baseline offset: for small font baseline~8, large baseline~12
    const topY = baselineY - (large ? 12 : 8);

    for (let py = 0; py < h; py++) {
      for (let px = 0; px < bmp.width; px++) {
        const alpha = bmp.pixels[(py * bmp.width + px) * 4 + 3];
        if (alpha > 100) {
          setPixel(x + px, topY + py, color[0], color[1], color[2]);
        }
      }
    }
    return x + bmp.width;
  }

  // Draw a filled rectangle
  function fillRect(x, y, w, h, color) {
    for (let dy = 0; dy < h; dy++)
      for (let dx = 0; dx < w; dx++)
        setPixel(x + dx, y + dy, color[0], color[1], color[2]);
  }

  // --- Score data model ---

  let scoreData = null;
  let scrollX = 0;
  let totalStripWidth = 0;
  let cards = [];

  function statusColor(status) {
    if (status === "in_progress") return GREEN;
    if (status === "halftime") return YELLOW;
    if (status === "final") return RED;
    if (status === "delayed") return ORANGE;
    return GRAY;
  }

  function sportColor(sport) {
    return SPORT_COLORS[sport] || DIM;
  }

  function sportLabel(sport) {
    return SPORT_LABELS[sport] || "???";
  }

  function textWidth(text, large) {
    const bmp = getTextBitmap(text, large);
    return bmp.width;
  }

  // Calculate width of a game card (match C++ calc_card_width)
  function calcCardWidth(game) {
    let w = LOGO_SIZE + 4;

    w += textWidth(game.away_team.abbreviation, false) + 4;

    if (game.home_team.score !== null && game.home_team.score !== undefined) {
      w += textWidth(String(game.away_team.score), false) + 4;
      w += 7; // " - "
      w += textWidth(String(game.home_team.score), false) + 4;
    }

    w += textWidth(game.home_team.abbreviation, false) + 8;

    if (game.detail) w += textWidth(game.detail, false) + 8;

    if (game.odds && game.odds.spread) {
      w += textWidth(game.odds.spread, false) + 4;
      if (game.odds.over_under)
        w += textWidth(game.odds.over_under, false) + 8;
    }

    if (game.venue) {
      const vshort = game.venue.substring(0, 24);
      w += textWidth(vshort, false) + 8;
    }

    w += GAME_CARD_GAP;
    return w;
  }

  function updateGames(data) {
    if (!data || !data.scoreboards) return;
    scoreData = data;
    cards = [];
    totalStripWidth = 0;

    let prevSport = "";
    for (const board of data.scoreboards) {
      for (const game of board.games) {
        game.sport = board.sport;

        if (game.sport !== prevSport) {
          totalStripWidth += SPORT_DIVIDER_W;
          prevSport = game.sport;
        }

        const cardWidth = calcCardWidth(game);
        cards.push({ game, totalWidth: cardWidth });
        totalStripWidth += cardWidth;
      }
    }

    if (totalStripWidth < DISPLAY_W) totalStripWidth = DISPLAY_W;
  }

  // --- Rendering (match C++ ticker.cpp) ---

  function drawSportDivider(x, sport) {
    const bg = sportColor(sport);
    fillRect(x, 0, SPORT_DIVIDER_W, 64, bg);
    drawText(x + 2, 36, WHITE, sportLabel(sport), false);
  }

  // Simple team logo placeholder (colored square with letter)
  function drawLogo(x, y, team, sport) {
    const sc = sportColor(sport);
    // Draw a small colored square
    for (let dy = 0; dy < LOGO_SIZE; dy++) {
      for (let dx = 0; dx < LOGO_SIZE; dx++) {
        // Border
        if (dx === 0 || dx === LOGO_SIZE - 1 || dy === 0 || dy === LOGO_SIZE - 1) {
          setPixel(x + dx, y + dy, sc[0], sc[1], sc[2]);
        } else {
          // Inner slightly lighter
          setPixel(x + dx, y + dy,
            Math.min(255, sc[0] + 30),
            Math.min(255, sc[1] + 30),
            Math.min(255, sc[2] + 30));
        }
      }
    }
    // Draw team abbreviation initial centered
    const initial = team.substring(0, 3);
    drawText(x + 3, y + 18, WHITE, initial, false);
  }

  function renderGameCard(card, xStart) {
    const g = card.game;
    let x = xStart;

    // Logo
    drawLogo(x, 2, g.away_team.abbreviation, g.sport);
    x += LOGO_SIZE + 4;

    // Line 1: Teams and scores (baseline y=20)
    const line1Y = 20;

    // Away rank
    if (g.away_team.rank && g.away_team.rank > 0) {
      x = drawText(x, line1Y - 4, YELLOW, "#" + g.away_team.rank, false);
      x += 2;
    }

    // Away abbreviation
    x = drawText(x, line1Y, WHITE, g.away_team.abbreviation, true);
    x += 4;

    // Scores
    const awayScore = g.away_team.score;
    const homeScore = g.home_team.score;
    if (awayScore !== null && awayScore !== undefined &&
        homeScore !== null && homeScore !== undefined) {
      const awayWinning = awayScore > homeScore;
      const homeWinning = homeScore > awayScore;

      x = drawText(x, line1Y, awayWinning ? WHITE : GRAY, String(awayScore), true);
      x += 3;
      x = drawText(x, line1Y, DIM, "-", true);
      x += 3;
      x = drawText(x, line1Y, homeWinning ? WHITE : GRAY, String(homeScore), true);
      x += 4;
    }

    // Home rank
    if (g.home_team.rank && g.home_team.rank > 0) {
      x = drawText(x, line1Y - 4, YELLOW, "#" + g.home_team.rank, false);
      x += 2;
    }

    // Home abbreviation
    x = drawText(x, line1Y, WHITE, g.home_team.abbreviation, true);
    x += 6;

    // Line 2: Status, odds, venue (baseline y=48)
    let line2X = xStart + LOGO_SIZE + 4;
    const line2Y = 48;
    const stc = statusColor(g.status);

    if (g.status === "in_progress" || g.status === "halftime") {
      if (g.period) {
        line2X = drawText(line2X, line2Y, stc, g.period, false);
        line2X += 4;
      }
      if (g.clock) {
        line2X = drawText(line2X, line2Y, WHITE, g.clock, false);
        line2X += 8;
      }
      if (g.status === "halftime") {
        line2X = drawText(line2X, line2Y, YELLOW, "HALF", false);
        line2X += 8;
      }
    } else if (g.status === "final") {
      line2X = drawText(line2X, line2Y, RED, "FINAL", false);
      line2X += 8;
    } else if (g.status === "scheduled") {
      if (g.detail) {
        line2X = drawText(line2X, line2Y, CYAN, g.detail, false);
        line2X += 8;
      }
    } else if (g.detail) {
      line2X = drawText(line2X, line2Y, stc, g.detail, false);
      line2X += 8;
    }

    // Betting odds
    if (g.odds) {
      drawText(line2X - 4, line2Y, DIM, "|", false);
      if (g.odds.spread) {
        line2X = drawText(line2X, line2Y, ORANGE, g.odds.spread, false);
        line2X += 4;
      }
      if (g.odds.over_under) {
        line2X = drawText(line2X, line2Y, ORANGE, g.odds.over_under, false);
        line2X += 8;
      }
    }

    // Venue
    if (g.venue) {
      drawText(line2X - 4, line2Y, DIM, "|", false);
      const venueShort = g.venue.substring(0, 24);
      line2X = drawText(line2X, line2Y, GRAY, venueShort, false);
    }

    return Math.max(x, line2X) - xStart + GAME_CARD_GAP;
  }

  function renderFrame() {
    clearBuffer();

    if (cards.length === 0) {
      drawText(40, 36, CYAN, "No games right now", true);
      return;
    }

    // Render strip twice for seamless looping (match C++)
    let stripX = -scrollX;

    for (let pass = 0; pass < 2; pass++) {
      let cx = stripX;
      let prevSport = "";

      for (const card of cards) {
        // Sport divider
        if (card.game.sport !== prevSport) {
          if (cx + SPORT_DIVIDER_W > 0 && cx < DISPLAY_W)
            drawSportDivider(cx, card.game.sport);
          cx += SPORT_DIVIDER_W;
          prevSport = card.game.sport;
        }

        // Only render if visible
        if (cx + card.totalWidth > 0 && cx < DISPLAY_W)
          renderGameCard(card, cx);
        cx += card.totalWidth;
      }

      stripX += totalStripWidth;
    }
  }

  // --- Canvas output ---

  let canvas, ctx;
  let pixelScale = 3;
  let showGrid = false;

  function blitToCanvas() {
    const w = DISPLAY_W * pixelScale;
    const h = DISPLAY_H * pixelScale;

    if (canvas.width !== w || canvas.height !== h) {
      canvas.width = w;
      canvas.height = h;
    }

    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, w, h);

    for (let y = 0; y < DISPLAY_H; y++) {
      for (let x = 0; x < DISPLAY_W; x++) {
        const idx = (y * DISPLAY_W + x) * 3;
        const r = pixelBuffer[idx];
        const g = pixelBuffer[idx + 1];
        const b = pixelBuffer[idx + 2];

        if (r === 0 && g === 0 && b === 0) continue;

        ctx.fillStyle = `rgb(${r},${g},${b})`;

        if (showGrid) {
          // Leave 1px gap between pixels for grid effect
          ctx.fillRect(
            x * pixelScale + 1,
            y * pixelScale + 1,
            pixelScale - 1,
            pixelScale - 1
          );
        } else {
          ctx.fillRect(
            x * pixelScale,
            y * pixelScale,
            pixelScale,
            pixelScale
          );
        }
      }
    }

    // Draw grid lines if enabled
    if (showGrid && pixelScale >= 3) {
      ctx.strokeStyle = "rgba(40,40,40,0.5)";
      ctx.lineWidth = 1;
      for (let x = 0; x <= DISPLAY_W; x++) {
        ctx.beginPath();
        ctx.moveTo(x * pixelScale + 0.5, 0);
        ctx.lineTo(x * pixelScale + 0.5, h);
        ctx.stroke();
      }
      for (let y = 0; y <= DISPLAY_H; y++) {
        ctx.beginPath();
        ctx.moveTo(0, y * pixelScale + 0.5);
        ctx.lineTo(w, y * pixelScale + 0.5);
        ctx.stroke();
      }
    }
  }

  // --- Animation loop ---

  let scrollSpeed = 1;
  let frameCount = 0;
  let lastFpsTime = 0;

  function advance() {
    scrollX += scrollSpeed;
    if (totalStripWidth > 0 && scrollX >= totalStripWidth) {
      scrollX -= totalStripWidth;
    }
  }

  function animationLoop(timestamp) {
    renderFrame();
    blitToCanvas();
    advance();

    // FPS counter
    frameCount++;
    if (timestamp - lastFpsTime >= 1000) {
      const fps = Math.round(frameCount * 1000 / (timestamp - lastFpsTime));
      document.getElementById("fps-display").textContent = fps + " FPS";
      frameCount = 0;
      lastFpsTime = timestamp;
    }

    requestAnimationFrame(animationLoop);
  }

  // --- Score fetching ---

  let fetchInterval = 30000; // ms

  async function fetchScores() {
    try {
      const resp = await fetch("/api/scores");
      if (!resp.ok) return;
      const data = await resp.json();
      updateGames(data);
    } catch (e) {
      console.warn("Failed to fetch scores:", e);
    }
  }

  let scoreTimer = null;
  function startScoreFetcher() {
    fetchScores();
    if (scoreTimer) clearInterval(scoreTimer);
    scoreTimer = setInterval(fetchScores, fetchInterval);
  }

  // --- Config editor ---

  async function loadConfig() {
    try {
      const resp = await fetch("/api/config");
      if (!resp.ok) return;
      const config = await resp.json();
      applyConfigToUI(config);
    } catch (e) {
      console.warn("Failed to load config:", e);
    }
  }

  function applyConfigToUI(config) {
    // Sports checkboxes
    document.querySelectorAll('input[name="sport"]').forEach(cb => {
      cb.checked = config.sports && config.sports.includes(cb.value);
    });

    // Scroll speed
    const scrollEl = document.getElementById("scroll-speed");
    if (scrollEl && config.scroll_speed !== undefined) {
      scrollEl.value = config.scroll_speed;
      document.getElementById("scroll-speed-val").textContent = config.scroll_speed;
      scrollSpeed = config.scroll_speed;
    }

    // Brightness
    const brightEl = document.getElementById("brightness");
    if (brightEl && config.brightness !== undefined) {
      brightEl.value = config.brightness;
      document.getElementById("brightness-val").textContent = config.brightness + "%";
    }

    // Update interval
    const updateEl = document.getElementById("update-interval");
    if (updateEl && config.update_interval_seconds !== undefined) {
      updateEl.value = config.update_interval_seconds;
      document.getElementById("update-interval-val").textContent = config.update_interval_seconds + "s";
      fetchInterval = config.update_interval_seconds * 1000;
    }

    // Timezone
    const tzEl = document.getElementById("timezone");
    if (tzEl && config.timezone) {
      tzEl.value = config.timezone;
    }

    // Content toggles
    const bettingEl = document.getElementById("show-betting");
    if (bettingEl && config.show_betting !== undefined)
      bettingEl.checked = config.show_betting;

    const venueEl = document.getElementById("show-venue");
    if (venueEl && config.show_venue !== undefined)
      venueEl.checked = config.show_venue;
  }

  function gatherConfigFromUI() {
    const sports = [];
    document.querySelectorAll('input[name="sport"]:checked').forEach(cb => {
      sports.push(cb.value);
    });

    return {
      sports,
      scroll_speed: parseInt(document.getElementById("scroll-speed").value, 10),
      brightness: parseInt(document.getElementById("brightness").value, 10),
      update_interval_seconds: parseInt(document.getElementById("update-interval").value, 10),
      timezone: document.getElementById("timezone").value,
      show_betting: document.getElementById("show-betting").checked,
      show_venue: document.getElementById("show-venue").checked,
    };
  }

  function showStatus(msg, isError) {
    const el = document.getElementById("config-status");
    el.textContent = msg;
    el.className = isError ? "error" : "success";
    if (!isError) {
      setTimeout(() => { el.textContent = ""; el.className = ""; }, 3000);
    }
  }

  async function saveConfig() {
    const config = gatherConfigFromUI();
    try {
      const resp = await fetch("/api/config", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(config),
      });
      const data = await resp.json();
      if (!resp.ok) {
        showStatus("Error: " + (data.error || "Save failed"), true);
        return;
      }
      showStatus("Configuration saved.", false);
      scrollSpeed = config.scroll_speed;
      fetchInterval = config.update_interval_seconds * 1000;
      startScoreFetcher();
    } catch (e) {
      showStatus("Error: " + e.message, true);
    }
  }

  async function resetConfig() {
    try {
      const resp = await fetch("/api/config/reset", { method: "POST" });
      const data = await resp.json();
      if (!resp.ok) {
        showStatus("Error: " + (data.error || "Reset failed"), true);
        return;
      }
      applyConfigToUI(data);
      showStatus("Configuration reset to defaults.", false);
      scrollSpeed = data.scroll_speed || 1;
      fetchInterval = (data.update_interval_seconds || 30) * 1000;
      startScoreFetcher();
    } catch (e) {
      showStatus("Error: " + e.message, true);
    }
  }

  // --- Init ---

  function init() {
    // Init pixel buffer
    pixelBuffer = new Uint8Array(DISPLAY_W * DISPLAY_H * 3);

    // Init font renderers
    initFontRenderers();

    // Canvas setup
    canvas = document.getElementById("led-canvas");
    ctx = canvas.getContext("2d");
    ctx.imageSmoothingEnabled = false;

    // Pixel scale control
    document.getElementById("pixel-scale").addEventListener("change", function () {
      pixelScale = parseInt(this.value, 10);
    });

    // Grid toggle
    document.getElementById("show-grid").addEventListener("change", function () {
      showGrid = this.checked;
    });

    // Range slider live value display
    document.getElementById("scroll-speed").addEventListener("input", function () {
      document.getElementById("scroll-speed-val").textContent = this.value;
    });
    document.getElementById("brightness").addEventListener("input", function () {
      document.getElementById("brightness-val").textContent = this.value + "%";
    });
    document.getElementById("update-interval").addEventListener("input", function () {
      document.getElementById("update-interval-val").textContent = this.value + "s";
    });

    // Config buttons
    document.getElementById("btn-save").addEventListener("click", saveConfig);
    document.getElementById("btn-reset").addEventListener("click", resetConfig);

    // Load config and start
    loadConfig();
    startScoreFetcher();

    // Start animation
    lastFpsTime = performance.now();
    requestAnimationFrame(animationLoop);
  }

  // Wait for DOM
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
