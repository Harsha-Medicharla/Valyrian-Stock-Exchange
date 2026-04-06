const fs = require('fs');
const path = require('path');

const dir = path.join('c:', 'Coding', 'Valyrian-Stock-Exchange', 'frontend', 'src');

function getFiles(basePath, arrayOfFiles) {
    const files = fs.readdirSync(basePath);
    files.forEach((file) => {
        if (fs.statSync(basePath + "/" + file).isDirectory()) {
            arrayOfFiles = getFiles(basePath + "/" + file, arrayOfFiles);
        } else {
            if (file.endsWith('.jsx')) {
                arrayOfFiles.push(path.join(basePath, "/", file));
            }
        }
    });
    return arrayOfFiles;
}

const allJsx = getFiles(dir, []);

allJsx.forEach(f => {
    let content = fs.readFileSync(f, 'utf8');

    // Remove emojis
    content = content.replace(/📊/g, '');
    content = content.replace(/🏛️/g, '');
    content = content.replace(/⚡/g, '');
    content = content.replace(/💼/g, '');
    content = content.replace(/👤/g, '');
    content = content.replace(/↩/g, '');
    content = content.replace(/💰/g, '');
    content = content.replace(/👋/g, '');
    content = content.replace(/📈/g, '');
    content = content.replace(/📉/g, '');
    content = content.replace(/📭/g, '');
    content = content.replace(/📝/g, '');
    content = content.replace(/📋/g, '');
    content = content.replace(/🟢/g, '');
    content = content.replace(/🔴/g, '');
    content = content.replace(/✅/g, '');
    content = content.replace(/🔍/g, '');

    // some places might have trailing spaces after emoji removal
    content = content.replace(/' /g, "'");
    content = content.replace(/ >/g, ">");
    content = content.replace(/> /g, ">");

    // Let's just fix the button texts in TradePage specifically
    if (f.includes('TradePage.jsx')) {
        content = content.replace(/> \s*{side} {selectedSymbol}/, ">{side} {selectedSymbol}");
    }

    fs.writeFileSync(f, content, 'utf8');
});

console.log('JSX emojis removed.');
