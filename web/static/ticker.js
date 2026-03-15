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
  const LOGO_SIZE = 60;

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

  function formatHours(h) {
    if (h === 0) return "0h";
    if (h < 24) return h + "h";
    const days = Math.floor(h / 24);
    const rem = h % 24;
    return rem === 0 ? days + "d" : days + "d " + rem + "h";
  }

  // --- Bitmap font (6x10) ---
  // Minimal bitmap font for ASCII 32-127, matching 6x10 BDF
  // Each character is 6 pixels wide, 10 pixels tall
  // Stored as array of 10 rows of 6-bit values per character

  const FONT_W = 6;
  const FONT_H = 14;
  const LARGE_FONT_W = 10;
  const LARGE_FONT_H = 24;
  const TINY_FONT_H = 11;

  // We'll use canvas-based text rendering scaled to pixel grid
  // This gives us a clean bitmap look while supporting full ASCII

  let fontCanvas, fontCtx;
  let largeFontCanvas, largeFontCtx;
  let tinyFontCanvas, tinyFontCtx;

  function initFontRenderers() {
    fontCanvas = document.createElement("canvas");
    fontCanvas.width = 512;
    fontCanvas.height = 18;
    fontCtx = fontCanvas.getContext("2d");
    fontCtx.imageSmoothingEnabled = false;

    largeFontCanvas = document.createElement("canvas");
    largeFontCanvas.width = 512;
    largeFontCanvas.height = 32;
    largeFontCtx = largeFontCanvas.getContext("2d");
    largeFontCtx.imageSmoothingEnabled = false;

    tinyFontCanvas = document.createElement("canvas");
    tinyFontCanvas.width = 512;
    tinyFontCanvas.height = 14;
    tinyFontCtx = tinyFontCanvas.getContext("2d");
    tinyFontCtx.imageSmoothingEnabled = false;
  }

  // Render text to a small off-screen canvas to get bitmap data
  // large=true: 20px font  large=false: 12px font  tiny=true: 9px font
  function getTextBitmap(text, large, tiny) {
    const ctx = large ? largeFontCtx : (tiny ? tinyFontCtx : fontCtx);
    const cvs = large ? largeFontCanvas : (tiny ? tinyFontCanvas : fontCanvas);
    const h   = large ? LARGE_FONT_H   : (tiny ? TINY_FONT_H   : FONT_H);
    const fontSize = large ? 20 : (tiny ? 9 : 12);

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

  function drawText(x, baselineY, color, text, large, tiny) {
    const bmp = getTextBitmap(text, large, tiny);
    if (!bmp.pixels) return x;
    const h = bmp.height;
    // baseline offset: large~17, small~10, tiny~7
    const topY = baselineY - (large ? 17 : (tiny ? 7 : 10));

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
  let showBetting = true;
  let showBracket = false;
  let bracketData = null;
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

  function textWidth(text, large, tiny) {
    const bmp = getTextBitmap(text, large, tiny);
    return bmp.width;
  }

  // Returns the x that centers `w` pixels within [lo, hi]
  function centerX(lo, hi, w) {
    return Math.floor(lo + (hi - lo - w) / 2);
  }

  // Calculate width of a game card (match C++ calc_card_width)
  function calcCardWidth(game) {
    // Line 1: teams + scores
    let line1 = 0;
    if (game.away_team.rank && game.away_team.rank > 0)
      line1 += textWidth("#" + game.away_team.rank, false) + 2;
    line1 += textWidth(game.away_team.abbreviation, true) + 4;

    const hasScores = game.status !== "scheduled" &&
                      game.home_team.score !== null && game.home_team.score !== undefined &&
                      game.away_team.score !== null && game.away_team.score !== undefined;
    if (hasScores) {
      line1 += textWidth(String(game.away_team.score), true) + 3;
      line1 += textWidth("@", true) + 6;
      line1 += textWidth(String(game.home_team.score), true) + 4;
    } else {
      line1 += textWidth("@", true) + 6;
    }
    if (game.home_team.rank && game.home_team.rank > 0)
      line1 += textWidth("#" + game.home_team.rank, false) + 2;
    line1 += textWidth(game.home_team.abbreviation, true) + 8;

    // Line 2: status/time (tiny font for scheduled)
    let line2 = 0;
    if (game.status === "scheduled" && game.detail) {
      line2 = textWidth(game.detail, false, true) + 8;  // tiny font
    } else if (game.status === "final") {
      line2 = textWidth("FINAL", false) + 8;
    } else if ((game.status === "in_progress" || game.status === "halftime") &&
               (game.period || game.clock)) {
      if (game.period) line2 += textWidth(game.period, false) + 4;
      if (game.clock)  line2 += textWidth(game.clock, false) + 4;
      if (game.status === "halftime") line2 += textWidth("HALF", false) + 4;
    } else if (game.detail) {
      line2 = textWidth(game.detail, false) + 8;
    }

    // Line 3: bet results for final, raw odds otherwise
    let line3 = 0;
    if (game.status === "final" && game.bet_results) {
      const br = game.bet_results;
      let bw = 0;
      if (br.spread_text) bw += textWidth(br.spread_text, false);
      if (br.ou_result) {
        if (bw > 0) bw += 10;
        const letter = br.ou_result === "OVER" ? "O" : br.ou_result === "UNDER" ? "U" : "P";
        const lineStr = br.ou_line === Math.floor(br.ou_line)
          ? String(Math.floor(br.ou_line)) : br.ou_line.toFixed(1);
        bw += textWidth(letter + " " + lineStr, false);
      }
      if (br.winner_ml && br.winner_abbr) {
        if (bw > 0) bw += 10;
        bw += textWidth(br.winner_abbr + " " + br.winner_ml, false);
      }
      line3 = bw;
    } else if (game.odds && game.odds.spread) {
      line3 = textWidth(game.odds.spread, false) + 8;
      if (game.odds.over_under)
        line3 += textWidth("  " + game.odds.over_under, false);
    }

    const textW = Math.max(line1, line2, line3);
    return LOGO_SIZE + 4 + textW + 4 + LOGO_SIZE + GAME_CARD_GAP;
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

  // --- Team logo loading ---
  // Fetches actual ESPN logos via the /api/logo proxy, caches rendered pixels.

  const logoPixelCache = new Map();  // key → Uint8ClampedArray (LOGO_SIZE*LOGO_SIZE*4) or "loading"

  function loadLogo(team, sport, logoUrl) {
    const key = sport + "_" + team;
    if (logoPixelCache.has(key)) return;
    if (!logoUrl) return;

    logoPixelCache.set(key, "loading");
    const img = new Image();
    img.onload = () => {
      const srcW = img.naturalWidth || img.width;
      const srcH = img.naturalHeight || img.height;

      // Find bounding box of non-transparent pixels so all logos fill the same visual size
      const src = document.createElement("canvas");
      src.width = srcW;
      src.height = srcH;
      const sctx = src.getContext("2d");
      sctx.drawImage(img, 0, 0);
      let srcData;
      try {
        srcData = sctx.getImageData(0, 0, srcW, srcH).data;
      } catch (e) {
        logoPixelCache.delete(key);
        return;
      }

      let minX = srcW, minY = srcH, maxX = 0, maxY = 0;
      for (let y = 0; y < srcH; y++) {
        for (let x = 0; x < srcW; x++) {
          if (srcData[(y * srcW + x) * 4 + 3] > 10) {
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
          }
        }
      }
      if (maxX <= minX || maxY <= minY) { minX = 0; minY = 0; maxX = srcW - 1; maxY = srcH - 1; }

      const contentW = maxX - minX + 1;
      const contentH = maxY - minY + 1;
      const pad = 2;
      const scale = Math.min((LOGO_SIZE - pad * 2) / contentW, (LOGO_SIZE - pad * 2) / contentH);
      const dstW = Math.round(contentW * scale);
      const dstH = Math.round(contentH * scale);
      const dstX = Math.round((LOGO_SIZE - dstW) / 2);
      const dstY = Math.round((LOGO_SIZE - dstH) / 2);

      const tmp = document.createElement("canvas");
      tmp.width = LOGO_SIZE;
      tmp.height = LOGO_SIZE;
      const tctx = tmp.getContext("2d");
      tctx.imageSmoothingEnabled = true;
      tctx.drawImage(img, minX, minY, contentW, contentH, dstX, dstY, dstW, dstH);
      try {
        logoPixelCache.set(key, tctx.getImageData(0, 0, LOGO_SIZE, LOGO_SIZE).data);
      } catch (e) {
        logoPixelCache.delete(key);
      }
    };
    img.onerror = () => logoPixelCache.delete(key);
    img.src = "/api/logo?url=" + encodeURIComponent(logoUrl);
  }

  function drawLogoFallback(x, y, team, sport) {
    const sc = sportColor(sport);
    for (let dy = 0; dy < LOGO_SIZE; dy++) {
      for (let dx = 0; dx < LOGO_SIZE; dx++) {
        if (dx === 0 || dx === LOGO_SIZE - 1 || dy === 0 || dy === LOGO_SIZE - 1) {
          setPixel(x + dx, y + dy, sc[0], sc[1], sc[2]);
        } else {
          setPixel(x + dx, y + dy,
            Math.min(255, sc[0] + 30),
            Math.min(255, sc[1] + 30),
            Math.min(255, sc[2] + 30));
        }
      }
    }
    drawText(x + 3, y + Math.floor(LOGO_SIZE * 0.6), WHITE, team.substring(0, 3), false);
  }

  function drawLogo(x, y, team, sport, logoUrl) {
    const key = sport + "_" + team;
    const cached = logoPixelCache.get(key);

    if (cached && cached !== "loading") {
      for (let py = 0; py < LOGO_SIZE; py++) {
        for (let px = 0; px < LOGO_SIZE; px++) {
          const i = (py * LOGO_SIZE + px) * 4;
          if (cached[i + 3] > 50) {
            setPixel(x + px, y + py, cached[i], cached[i + 1], cached[i + 2]);
          }
        }
      }
      return;
    }

    // Trigger fetch if not already loading
    if (!cached && logoUrl) loadLogo(team, sport, logoUrl);
    drawLogoFallback(x, y, team, sport);
  }

  function renderGameCard(card, xStart) {
    const g = card.game;

    // Away logo (left, full height)
    drawLogo(xStart, 2, g.away_team.abbreviation, g.sport, g.away_team.logo_url);

    const textStartX = xStart + LOGO_SIZE + 4;

    // Text area dimensions from pre-calculated card width
    // calcCardWidth: LOGO_SIZE + 4 + textW + 4 + LOGO_SIZE + GAME_CARD_GAP
    const textW = card.totalWidth - 2 * LOGO_SIZE - 8 - GAME_CARD_GAP;
    const textEndX = textStartX + textW;

    // Line 1: Teams and scores (large font, baseline y=26)
    const line1Y = 26;
    const awayScore = g.away_team.score;
    const homeScore = g.home_team.score;
    const hasScores = g.status !== "scheduled" && awayScore != null && homeScore != null;

    // Measure line1 width to center it between the logos
    let line1W = 0;
    if (g.away_team.rank > 0) line1W += textWidth("#" + g.away_team.rank, false) + 2;
    line1W += textWidth(g.away_team.abbreviation, true) + 4;
    if (hasScores) {
      line1W += textWidth(String(awayScore), true) + 3 + textWidth("@", true) + 3 + textWidth(String(homeScore), true) + 4;
    } else {
      line1W += textWidth("@", true) + 4;
    }
    if (g.home_team.rank > 0) line1W += textWidth("#" + g.home_team.rank, false) + 2;
    line1W += textWidth(g.home_team.abbreviation, true);

    let x = centerX(textStartX, textEndX, line1W);

    if (g.away_team.rank && g.away_team.rank > 0) {
      x = drawText(x, line1Y - 6, YELLOW, "#" + g.away_team.rank, false);
      x += 2;
    }
    x = drawText(x, line1Y, WHITE, g.away_team.abbreviation, true);
    x += 4;
    if (hasScores) {
      const awayWinning = awayScore > homeScore;
      const homeWinning = homeScore > awayScore;
      x = drawText(x, line1Y, awayWinning ? WHITE : GRAY, String(awayScore), true);
      x += 3;
      x = drawText(x, line1Y, DIM, "@", true);
      x += 3;
      x = drawText(x, line1Y, homeWinning ? WHITE : GRAY, String(homeScore), true);
      x += 4;
    } else {
      x = drawText(x, line1Y, DIM, "@", true);
      x += 4;
    }
    if (g.home_team.rank && g.home_team.rank > 0) {
      x = drawText(x, line1Y - 6, YELLOW, "#" + g.home_team.rank, false);
      x += 2;
    }
    x = drawText(x, line1Y, WHITE, g.home_team.abbreviation, true);

    const stc = statusColor(g.status);

    // --- Line 2: Status/time, centered (baseline y=43) ---
    const line2Y = 43;

    if (g.status === "in_progress" || g.status === "halftime") {
      // Build parts: period, clock, HALF — centered together under the score
      const parts = [];
      if (g.period) parts.push({ text: g.period, color: stc });
      if (g.clock)  parts.push({ text: g.clock,  color: WHITE });
      if (g.status === "halftime") parts.push({ text: "HALF", color: YELLOW });
      const totalW = parts.reduce((s, p, i) =>
        s + textWidth(p.text, false) + (i < parts.length - 1 ? 4 : 0), 0);
      let cx = centerX(textStartX, textEndX, totalW);
      parts.forEach((p, i) => {
        cx = drawText(cx, line2Y, p.color, p.text, false);
        if (i < parts.length - 1) cx += 4;
      });
    } else if (g.status === "final") {
      const fw = textWidth("FINAL", false);
      drawText(centerX(textStartX, textEndX, fw), line2Y, RED, "FINAL", false);
    } else if (g.status === "scheduled" && g.detail) {
      // Tiny (9px) font — scheduled detail strings can be very long
      const dw = textWidth(g.detail, false, true);
      drawText(centerX(textStartX, textEndX, dw), line2Y, CYAN, g.detail, false, true);
    } else if (g.detail) {
      const dw = textWidth(g.detail, false);
      drawText(centerX(textStartX, textEndX, dw), line2Y, stc, g.detail, false);
    }

    // --- Line 3: Bet results for final, raw odds otherwise ---
    const line3Y = 56;
    if (g.status === "final" && g.bet_results) {
      const br = g.bet_results;
      const segments = [];

      if (br.spread_text) {
        const color = br.spread_result === "covered" ? GREEN
                    : br.spread_result === "push" ? YELLOW : RED;
        segments.push({ text: br.spread_text, color });
      }
      if (br.ou_result) {
        const letter = br.ou_result === "OVER" ? "O" : br.ou_result === "UNDER" ? "U" : "P";
        const lineStr = br.ou_line != null
          ? (br.ou_line === Math.floor(br.ou_line) ? String(Math.floor(br.ou_line)) : br.ou_line.toFixed(1))
          : "";
        segments.push({ text: letter + " " + lineStr, color: WHITE });
      }
      if (br.winner_ml && br.winner_abbr) {
        segments.push({ text: br.winner_abbr + " " + br.winner_ml, color: GREEN });
      }

      if (segments.length > 0) {
        const gap = 10;
        let totalW = 0;
        segments.forEach((s, i) => {
          totalW += textWidth(s.text, false);
          if (i < segments.length - 1) totalW += gap;
        });
        let bx = centerX(textStartX, textEndX, totalW);
        segments.forEach((s, i) => {
          bx = drawText(bx, line3Y, s.color, s.text, false);
          if (i < segments.length - 1) bx += gap;
        });
      }
    } else if (g.odds && showBetting) {
      let oddsText = "";
      if (g.odds.spread)     oddsText += g.odds.spread;
      if (g.odds.over_under) oddsText += (oddsText ? "  " : "") + g.odds.over_under;
      if (oddsText) {
        const ow = textWidth(oddsText, false);
        drawText(centerX(textStartX, textEndX, ow), line3Y, ORANGE, oddsText, false);
      }
    }

    // Home logo (right, full height)
    drawLogo(textEndX + 4, 2, g.home_team.abbreviation, g.sport, g.home_team.logo_url);

    return textEndX + 4 + LOGO_SIZE - xStart + GAME_CARD_GAP;
  }

  // --- Bracket rendering ---

  const BRACKET_REGION_COLORS = {
    "East":    [29, 66, 138],
    "West":    [128, 0, 0],
    "South":   [0, 100, 0],
    "Midwest": [139, 69, 19],
  };

  let bracketCards = [];
  let bracketStripWidth = 0;

  function updateBracket(data) {
    if (!data || !data.bracket || !data.bracket.matchups) return;
    bracketData = data.bracket;
    bracketCards = [];
    bracketStripWidth = 0;

    // Group matchups by region, then by round
    const matchups = bracketData.matchups;
    if (matchups.length === 0) return;

    let prevRegion = "";
    for (const m of matchups) {
      if (m.region && m.region !== prevRegion) {
        bracketStripWidth += SPORT_DIVIDER_W;
        prevRegion = m.region;
      }
      const cardWidth = calcBracketCardWidth(m);
      bracketCards.push({ matchup: m, totalWidth: cardWidth });
      bracketStripWidth += cardWidth;
    }

    if (bracketStripWidth < DISPLAY_W) bracketStripWidth = DISPLAY_W;
  }

  function calcBracketCardWidth(m) {
    // Line 1: "#seed AWAY score @ score HOME #seed"
    let line1 = 0;
    if (m.away_seed > 0) line1 += textWidth("#" + m.away_seed, false) + 2;
    if (m.away_team) line1 += textWidth(m.away_team.abbreviation, true) + 4;

    const hasScores = m.status !== "scheduled" &&
                      m.home_team && m.away_team &&
                      m.home_team.score != null && m.away_team.score != null;
    if (hasScores) {
      line1 += textWidth(String(m.away_team.score), true) + 3;
      line1 += textWidth("@", true) + 3;
      line1 += textWidth(String(m.home_team.score), true) + 4;
    } else {
      line1 += textWidth("vs", true) + 4;
    }

    if (m.home_seed > 0) line1 += textWidth("#" + m.home_seed, false) + 2;
    if (m.home_team) line1 += textWidth(m.home_team.abbreviation, true) + 8;

    // Line 2: round name + status
    let line2 = 0;
    if (m.status === "final") {
      line2 = textWidth("FINAL", false) + 8;
    } else if (m.status === "in_progress" || m.status === "halftime") {
      if (m.period) line2 += textWidth(m.period, false) + 4;
      if (m.clock) line2 += textWidth(m.clock, false) + 4;
    } else if (m.detail) {
      line2 = textWidth(m.detail, false, true) + 8;
    }

    // Line 3: round name
    let line3 = 0;
    if (m.round_name) line3 = textWidth(m.round_name, false) + 8;

    const textW = Math.max(line1, line2, line3);
    return LOGO_SIZE + 4 + textW + 4 + LOGO_SIZE + GAME_CARD_GAP;
  }

  function renderBracketCard(card, xStart) {
    const m = card.matchup;

    // Away logo
    if (m.away_team) {
      drawLogo(xStart, 2, m.away_team.abbreviation, "ncaam", m.away_team.logo_url);
    }

    const textStartX = xStart + LOGO_SIZE + 4;
    const textW = card.totalWidth - 2 * LOGO_SIZE - 8 - GAME_CARD_GAP;
    const textEndX = textStartX + textW;

    // Line 1: seeds + teams + scores
    const line1Y = 26;
    const hasScores = m.status !== "scheduled" &&
                      m.home_team && m.away_team &&
                      m.home_team.score != null && m.away_team.score != null;

    let line1W = 0;
    if (m.away_seed > 0) line1W += textWidth("#" + m.away_seed, false) + 2;
    if (m.away_team) line1W += textWidth(m.away_team.abbreviation, true) + 4;
    if (hasScores) {
      line1W += textWidth(String(m.away_team.score), true) + 3 +
                textWidth("@", true) + 3 +
                textWidth(String(m.home_team.score), true) + 4;
    } else {
      line1W += textWidth("vs", true) + 4;
    }
    if (m.home_seed > 0) line1W += textWidth("#" + m.home_seed, false) + 2;
    if (m.home_team) line1W += textWidth(m.home_team.abbreviation, true);

    let x = centerX(textStartX, textEndX, line1W);

    // Away seed
    if (m.away_seed > 0) {
      x = drawText(x, line1Y - 6, CYAN, "#" + m.away_seed, false);
      x += 2;
    }
    if (m.away_team) {
      x = drawText(x, line1Y, WHITE, m.away_team.abbreviation, true);
      x += 4;
    }

    if (hasScores) {
      const awayWinning = m.away_team.score > m.home_team.score;
      const homeWinning = m.home_team.score > m.away_team.score;
      x = drawText(x, line1Y, awayWinning ? WHITE : GRAY, String(m.away_team.score), true);
      x += 3;
      x = drawText(x, line1Y, DIM, "@", true);
      x += 3;
      x = drawText(x, line1Y, homeWinning ? WHITE : GRAY, String(m.home_team.score), true);
      x += 4;
    } else {
      x = drawText(x, line1Y, DIM, "vs", true);
      x += 4;
    }

    // Home seed
    if (m.home_seed > 0) {
      x = drawText(x, line1Y - 6, CYAN, "#" + m.home_seed, false);
      x += 2;
    }
    if (m.home_team) {
      x = drawText(x, line1Y, WHITE, m.home_team.abbreviation, true);
    }

    // Line 2: status
    const line2Y = 43;
    const stc = statusColor(m.status);
    if (m.status === "in_progress" || m.status === "halftime") {
      const parts = [];
      if (m.period) parts.push({ text: m.period, color: stc });
      if (m.clock) parts.push({ text: m.clock, color: WHITE });
      if (m.status === "halftime") parts.push({ text: "HALF", color: YELLOW });
      const totalW = parts.reduce((s, p, i) =>
        s + textWidth(p.text, false) + (i < parts.length - 1 ? 4 : 0), 0);
      let cx = centerX(textStartX, textEndX, totalW);
      parts.forEach((p, i) => {
        cx = drawText(cx, line2Y, p.color, p.text, false);
        if (i < parts.length - 1) cx += 4;
      });
    } else if (m.status === "final") {
      const fw = textWidth("FINAL", false);
      drawText(centerX(textStartX, textEndX, fw), line2Y, RED, "FINAL", false);
    } else if (m.detail) {
      const dw = textWidth(m.detail, false, true);
      drawText(centerX(textStartX, textEndX, dw), line2Y, CYAN, m.detail, false, true);
    }

    // Line 3: round name
    const line3Y = 56;
    if (m.round_name) {
      const rw = textWidth(m.round_name, false);
      const regionColor = BRACKET_REGION_COLORS[m.region] || ORANGE;
      drawText(centerX(textStartX, textEndX, rw), line3Y, regionColor, m.round_name, false);
    }

    // Home logo
    if (m.home_team) {
      drawLogo(textEndX + 4, 2, m.home_team.abbreviation, "ncaam", m.home_team.logo_url);
    }
  }

  function drawBracketRegionDivider(x, region) {
    const bg = BRACKET_REGION_COLORS[region] || DIM;
    fillRect(x, 0, SPORT_DIVIDER_W, 64, bg);
    const label = region.substring(0, 3).toUpperCase();
    drawText(x + 2, 36, WHITE, label, false);
  }

  function renderFrame() {
    clearBuffer();

    // Notification takes priority over normal rendering
    if (notifyPhase !== "none") {
      renderNotification();
      return;
    }

    // Bracket mode
    if (showBracket && bracketCards.length > 0) {
      let stripX = -Math.floor(scrollX);
      for (let pass = 0; pass < 2; pass++) {
        let cx = stripX;
        let prevRegion = "";
        for (const card of bracketCards) {
          if (card.matchup.region && card.matchup.region !== prevRegion) {
            if (cx + SPORT_DIVIDER_W > 0 && cx < DISPLAY_W)
              drawBracketRegionDivider(cx, card.matchup.region);
            cx += SPORT_DIVIDER_W;
            prevRegion = card.matchup.region;
          }
          if (cx + card.totalWidth > 0 && cx < DISPLAY_W)
            renderBracketCard(card, cx);
          cx += card.totalWidth;
        }
        stripX += bracketStripWidth;
      }
      return;
    }

    if (cards.length === 0) {
      drawText(40, 36, CYAN, "No games right now", true);
      return;
    }

    // Render strip twice for seamless looping (match C++)
    let stripX = -Math.floor(scrollX);

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

  // --- Notification state machine (mirrors C++ ticker) ---

  let notifyPhase = "none"; // "none", "active"
  let notifyQueue = [];     // [{game, bet_results}, ...]
  let notifyGame = null;
  let notifyBetResults = null;
  let notifyFlashCount = 3;
  let notifyDisplayFrames = 200; // 5s * 40fps
  let notifyFramesRemaining = 0;
  let notifyFlashRemaining = 0;
  let notifyFlashTimer = 0;
  let notifyFlashOn = false;

  const NOTIFY_BORDER_W = 3;

  function startNextNotification() {
    if (notifyQueue.length === 0) {
      notifyPhase = "none";
      notifyBetResults = null;
      return;
    }
    const entry = notifyQueue.shift();
    notifyGame = entry.game;
    notifyBetResults = entry.bet_results || null;
    notifyPhase = "active";
    notifyFramesRemaining = notifyDisplayFrames;
    notifyFlashRemaining = notifyFlashCount;
    notifyFlashTimer = 0;
    notifyFlashOn = true;
  }

  function queueNotification(game, betResults) {
    notifyQueue.push({ game, bet_results: betResults || null });
    if (notifyPhase === "none") startNextNotification();
  }

  function drawBorder(color) {
    for (let i = 0; i < NOTIFY_BORDER_W; i++) {
      for (let x = 0; x < DISPLAY_W; x++) {
        setPixel(x, i, color[0], color[1], color[2]);                       // top
        setPixel(x, DISPLAY_H - 1 - i, color[0], color[1], color[2]);       // bottom
      }
      for (let y = 0; y < DISPLAY_H; y++) {
        setPixel(i, y, color[0], color[1], color[2]);                       // left
        setPixel(DISPLAY_W - 1 - i, y, color[0], color[1], color[2]);       // right
      }
    }
  }

  function renderNotification() {
    if (!notifyGame) return;

    // Copy bet_results into game so renderGameCard draws them on row 3
    const gameCopy = Object.assign({}, notifyGame);
    if (notifyBetResults) gameCopy.bet_results = notifyBetResults;

    const card = { game: gameCopy, totalWidth: calcCardWidth(gameCopy) };
    const cardContentW = card.totalWidth - GAME_CARD_GAP;
    const xStart = Math.floor((DISPLAY_W - cardContentW) / 2);
    renderGameCard(card, xStart);

    // Draw flashing border on top if still flashing
    if (notifyFlashRemaining > 0 && notifyFlashOn) {
      drawBorder(WHITE);
    }
  }

  function advanceNotification() {
    notifyFramesRemaining--;
    if (notifyFramesRemaining <= 0) {
      startNextNotification();
      return;
    }

    // Advance border flash cycles
    if (notifyFlashRemaining > 0) {
      notifyFlashTimer++;
      if (notifyFlashTimer < 6) {
        notifyFlashOn = true;
      } else if (notifyFlashTimer < 12) {
        notifyFlashOn = false;
      } else {
        notifyFlashRemaining--;
        notifyFlashTimer = 0;
        notifyFlashOn = true;
      }
    }
  }

  // --- Animation loop ---

  let scrollSpeed = 1;
  let frameCount = 0;
  let lastFpsTime = 0;

  function advance() {
    if (notifyPhase !== "none") {
      advanceNotification();
      return;
    }
    scrollX += scrollSpeed / 4;
    const activeWidth = (showBracket && bracketCards.length > 0) ? bracketStripWidth : totalStripWidth;
    if (activeWidth > 0 && scrollX >= activeWidth) {
      scrollX -= activeWidth;
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
    if (showBracket) {
      try {
        const resp = await fetch("/api/bracket");
        if (!resp.ok) return;
        const data = await resp.json();
        updateBracket(data);
      } catch (e) {
        console.warn("Failed to fetch bracket:", e);
      }
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

    // Time window
    const hoursBackEl = document.getElementById("hours-back");
    if (hoursBackEl && config.hours_back !== undefined) {
      hoursBackEl.value = config.hours_back;
      document.getElementById("hours-back-val").textContent = formatHours(config.hours_back);
    }
    const hoursAheadEl = document.getElementById("hours-ahead");
    if (hoursAheadEl && config.hours_ahead !== undefined) {
      hoursAheadEl.value = config.hours_ahead;
      document.getElementById("hours-ahead-val").textContent = formatHours(config.hours_ahead);
    }

    // Timezone
    const tzEl = document.getElementById("timezone");
    if (tzEl && config.timezone) {
      tzEl.value = config.timezone;
    }

    // Content toggles
    const bettingEl = document.getElementById("show-betting");
    if (bettingEl && config.show_betting !== undefined) {
      bettingEl.checked = config.show_betting;
      showBetting = config.show_betting;
    }

    const venueEl = document.getElementById("show-venue");
    if (venueEl && config.show_venue !== undefined)
      venueEl.checked = config.show_venue;

    const bracketEl = document.getElementById("show-bracket");
    if (bracketEl && config.show_bracket !== undefined) {
      bracketEl.checked = config.show_bracket;
      showBracket = config.show_bracket;
    }

    // Notification settings
    const notifyEl = document.getElementById("notify-on-final");
    if (notifyEl && config.notify_on_final !== undefined)
      notifyEl.checked = config.notify_on_final;

    const flashCountEl = document.getElementById("notify-flash-count");
    if (flashCountEl && config.notify_flash_count !== undefined) {
      flashCountEl.value = config.notify_flash_count;
      document.getElementById("notify-flash-count-val").textContent = config.notify_flash_count;
      notifyFlashCount = config.notify_flash_count;
    }

    const displaySecsEl = document.getElementById("notify-display-seconds");
    if (displaySecsEl && config.notify_display_seconds !== undefined) {
      displaySecsEl.value = config.notify_display_seconds;
      document.getElementById("notify-display-seconds-val").textContent = config.notify_display_seconds + "s";
      notifyDisplayFrames = config.notify_display_seconds * 40;
    }
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
      hours_back: parseInt(document.getElementById("hours-back").value, 10),
      hours_ahead: parseInt(document.getElementById("hours-ahead").value, 10),
      timezone: document.getElementById("timezone").value,
      show_betting: document.getElementById("show-betting").checked,
      show_venue: document.getElementById("show-venue").checked,
      show_bracket: document.getElementById("show-bracket").checked,
      notify_on_final: document.getElementById("notify-on-final").checked,
      notify_flash_count: parseInt(document.getElementById("notify-flash-count").value, 10),
      notify_display_seconds: parseInt(document.getElementById("notify-display-seconds").value, 10),
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
      showBetting = config.show_betting;
      showBracket = config.show_bracket;
      notifyFlashCount = config.notify_flash_count;
      notifyDisplayFrames = config.notify_display_seconds * 40;
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
    document.getElementById("hours-back").addEventListener("input", function () {
      document.getElementById("hours-back-val").textContent = formatHours(parseInt(this.value, 10));
    });
    document.getElementById("hours-ahead").addEventListener("input", function () {
      document.getElementById("hours-ahead-val").textContent = formatHours(parseInt(this.value, 10));
    });

    // Notification slider live display
    document.getElementById("notify-flash-count").addEventListener("input", function () {
      document.getElementById("notify-flash-count-val").textContent = this.value;
    });
    document.getElementById("notify-display-seconds").addEventListener("input", function () {
      document.getElementById("notify-display-seconds-val").textContent = this.value + "s";
    });

    // Test notification button
    document.getElementById("btn-test-notify").addEventListener("click", async function () {
      try {
        const resp = await fetch("/api/test-notification", { method: "POST" });
        const data = await resp.json();
        if (!resp.ok) {
          showStatus("Error: " + (data.error || "No games available"), true);
          return;
        }
        // Queue notification in the simulator
        if (data.game) {
          // Ensure sport field is set on the game
          if (!data.game.sport && data.game.game_id) data.game.sport = "";
          notifyFlashCount = parseInt(document.getElementById("notify-flash-count").value, 10);
          notifyDisplayFrames = parseInt(document.getElementById("notify-display-seconds").value, 10) * 40;
          queueNotification(data.game, data.bet_results);
          showStatus("Test notification sent.", false);
        }
      } catch (e) {
        showStatus("Error: " + e.message, true);
      }
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
