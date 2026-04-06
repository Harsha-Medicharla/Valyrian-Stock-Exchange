const fs = require('fs');
const path = require('path');

const cssPath = path.join('c:', 'Coding', 'Valyrian-Stock-Exchange', 'frontend', 'src', 'index.css');
let cssStr = fs.readFileSync(cssPath, 'utf8');

// 1. Update css variables
cssStr = cssStr.replace(/--bg-deepest: #060a13;/, '--bg-deepest: #0f1115;');
cssStr = cssStr.replace(/--bg-base: #0a0e17;/, '--bg-base: #14171c;');
cssStr = cssStr.replace(/--bg-surface: #0d1321;/, '--bg-surface: #1e2229;');
cssStr = cssStr.replace(/--bg-elevated: #111827;/, '--bg-elevated: #272c36;');
cssStr = cssStr.replace(/--bg-card: #141b2d;/, '--bg-card: #1b1f27;');
cssStr = cssStr.replace(/--bg-hover: #1a2340;/, '--bg-hover: #2d3340;');
cssStr = cssStr.replace(/--bg-input: #0f1729;/, '--bg-input: #0b0d10;');

// Update colors
cssStr = cssStr.replace(/--accent-primary: #00d4ff;/, '--accent-primary: #3b82f6;');
cssStr = cssStr.replace(/--accent-primary-dim: rgba\(0, 212, 255, 0\.15\);/, '--accent-primary-dim: rgba(59, 130, 246, 0.15);');
cssStr = cssStr.replace(/--color-buy: #00e676;/, '--color-buy: #10b981;');
cssStr = cssStr.replace(/--color-buy-dim: rgba\(0, 230, 118, 0\.12\);/, '--color-buy-dim: rgba(16, 185, 129, 0.15);');
cssStr = cssStr.replace(/--color-sell: #ff1744;/, '--color-sell: #ef4444;');
cssStr = cssStr.replace(/--color-sell-dim: rgba\(255, 23, 68, 0\.12\);/, '--color-sell-dim: rgba(239, 68, 68, 0.15);');

// Update border radius to be more squarish
cssStr = cssStr.replace(/--radius-sm: 6px;/, '--radius-sm: 4px;');
cssStr = cssStr.replace(/--radius-md: 10px;/, '--radius-md: 6px;');
cssStr = cssStr.replace(/--radius-lg: 16px;/, '--radius-lg: 8px;');
cssStr = cssStr.replace(/--radius-xl: 24px;/, '--radius-xl: 12px;');

// Remove glow shadows
cssStr = cssStr.replace(/--shadow-glow-blue: [^;]+;/g, '--shadow-glow-blue: none;');
cssStr = cssStr.replace(/--shadow-glow-green: [^;]+;/g, '--shadow-glow-green: none;');
cssStr = cssStr.replace(/--shadow-glow-red: [^;]+;/g, '--shadow-glow-red: none;');

// Update .glass-card to solid background without blur
const glassOld = `\\.glass-card \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(glassOld), `.glass-card {
  background: var(--bg-card);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-sm);
  transition: border-color var(--transition-base), box-shadow var(--transition-base);
}`);

// Buttons without gradients
const btnPrimary = `\\.btn-primary \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(btnPrimary), `.btn-primary {
  background: var(--accent-primary);
  color: var(--text-inverse);
  box-shadow: var(--shadow-sm);
}`);
const btnBuy = `\\.btn-buy \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(btnBuy), `.btn-buy {
  background: var(--color-buy);
  color: var(--text-inverse);
  box-shadow: var(--shadow-sm);
}`);
const btnSell = `\\.btn-sell \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(btnSell), `.btn-sell {
  background: var(--color-sell);
  color: #fff;
  box-shadow: var(--shadow-sm);
}`);

// Button hovers
cssStr = cssStr.replace(/transform: translateY\(-1px\);/g, '');
cssStr = cssStr.replace(/box-shadow: 0 0 30px[^;]*;/g, 'box-shadow: var(--shadow-md);');

// Remove gradient animations
cssStr = cssStr.replace(/animation: bgPulse[^;]*;/g, '');

// Navbar background
const navBar = `\\.navbar \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(navBar), `.navbar {
  position: fixed;
  top: 0;
  left: var(--sidebar-width);
  right: 0;
  height: var(--navbar-height);
  background: var(--bg-surface);
  border-bottom: 1px solid var(--border-subtle);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 var(--space-xl);
  z-index: 90;
}`);

const logoMark = `\\.login-logo \\.logo-mark \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(logoMark), `.login-logo .logo-mark {
  width: 56px;
  height: 56px;
  background: var(--accent-primary);
  border-radius: var(--radius-lg);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-weight: 900;
  font-size: 1.5rem;
  color: #fff;
  margin-bottom: var(--space-md);
  box-shadow: var(--shadow-sm);
}`);

const sideLogo = `\\.sidebar-logo \\.logo-icon \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(sideLogo), `.sidebar-logo .logo-icon {
  width: 36px;
  height: 36px;
  background: var(--accent-primary);
  border-radius: var(--radius-md);
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 800;
  font-size: 1.1rem;
  color: #fff;
}`);

const avatarLogo = `\\.user-avatar \\{[\\s\\S]*?\\}`;
cssStr = cssStr.replace(new RegExp(avatarLogo), `.user-avatar {
  width: 34px;
  height: 34px;
  background: var(--accent-primary);
  border-radius: var(--radius-full);
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 700;
  font-size: 0.8rem;
  color: #fff;
}`);

fs.writeFileSync(cssPath, cssStr, 'utf8');
console.log('CSS updated successfully.');
