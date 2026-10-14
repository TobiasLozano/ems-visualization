import { spawn, execSync } from 'child_process';
import http from 'http';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const buildDir = path.resolve(__dirname, '../build');
const outputFile = path.resolve(__dirname, '../../EMS_Microgrid_DC_Manual_Usuario.pdf');
const staticCopy = path.resolve(__dirname, '../static/EMS_Microgrid_DC_Manual_Usuario.pdf');

if (!fs.existsSync(buildDir)) {
  console.log('📦 Directorio build no encontrado. Compilando Docusaurus...');
  execSync('npm run build', { stdio: 'inherit', cwd: path.resolve(__dirname, '..') });
}

// Simple static server
const mimeTypes = {
  '.html': 'text/html',
  '.js': 'text/javascript',
  '.css': 'text/css',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.woff2': 'font/woff2'
};

const server = http.createServer((req, res) => {
  let reqUrl = req.url.split('?')[0];
  let filePath = path.join(buildDir, reqUrl);

  if (fs.existsSync(filePath) && fs.statSync(filePath).isDirectory()) {
    filePath = path.join(filePath, 'index.html');
  } else if (!fs.existsSync(filePath) && fs.existsSync(filePath + '.html')) {
    filePath = filePath + '.html';
  }

  if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) {
    const ext = path.extname(filePath);
    res.writeHead(200, { 'Content-Type': mimeTypes[ext] || 'application/octet-stream' });
    fs.createReadStream(filePath).pipe(res);
  } else {
    // SPA fallback
    const fallback = path.join(buildDir, '404.html');
    if (fs.existsSync(fallback)) {
      res.writeHead(404, { 'Content-Type': 'text/html' });
      fs.createReadStream(fallback).pipe(res);
    } else {
      res.writeHead(404);
      res.end('Not found');
    }
  }
});

const PORT = 3456;
server.listen(PORT, async () => {
  console.log(`🌐 Servidor local iniciado en http://localhost:${PORT}`);
  const targetUrl = `http://localhost:${PORT}/documento-completo`;

  console.log(`📄 Generando PDF formal desde ${targetUrl}...`);

  const homeDir = process.env.HOME || '/home/tobias';
  const chromeCandidates = [
    process.env.CHROME_BIN,
    process.env.PUPPETEER_EXECUTABLE_PATH,
    path.join(homeDir, '.cache/puppeteer/chrome/linux-152.0.7977.54/chrome-linux64/chrome'),
    path.join(homeDir, '.cache/ms-playwright/chromium-1243/chrome-linux64/chrome'),
    path.join(homeDir, '.cache/ms-playwright/chromium-1193/chrome-linux/chrome'),
    '/usr/bin/google-chrome',
    '/usr/bin/google-chrome-stable',
    '/usr/bin/chromium',
    '/usr/bin/chromium-browser',
    '/snap/bin/chromium',
    '/var/lib/flatpak/exports/bin/com.google.Chrome'
  ].filter(Boolean);

  let chromePath = null;
  for (const candidate of chromeCandidates) {
    try {
      if (fs.existsSync(candidate)) {
        const real = fs.realpathSync(candidate);
        if (fs.existsSync(real)) {
          chromePath = candidate;
          break;
        }
      }
    } catch {
      // ignore
    }
  }

  try {
    if (!chromePath) {
      console.error('⚠️  No se encontró un ejecutable de Google Chrome o Chromium en las rutas estándar.');
      console.error('👉 Puedes definir la variable CHROME_BIN: CHROME_BIN=/ruta/a/chrome npm run export-pdf');
    } else {
      const profileDir = path.resolve(__dirname, '../.chrome-profile');
      fs.mkdirSync(profileDir, { recursive: true });

      const args = [
        '--headless=new',
        '--disable-gpu',
        '--no-sandbox',
        '--disable-setuid-sandbox',
        '--disable-dev-shm-usage',
        '--disable-background-networking',
        '--disable-component-update',
        '--disable-default-apps',
        '--disable-extensions',
        '--disable-sync',
        '--no-first-run',
        '--run-all-compositor-stages-before-draw',
        '--virtual-time-budget=3000',
        `--user-data-dir=${profileDir}`,
        `--crash-dumps-dir=${profileDir}`,
        `--print-to-pdf=${outputFile}`,
        '--no-pdf-header-footer',
        targetUrl
      ];

      console.log(`🚀 Usando binario: ${chromePath}`);
      execSync(`${chromePath} ${args.join(' ')}`, {
        stdio: 'inherit',
        timeout: 45000,
        env: {
          ...process.env,
          HOME: profileDir,
          XDG_CONFIG_HOME: profileDir,
          XDG_CACHE_HOME: profileDir
        }
      });
      console.log(`✅ ¡PDF generado con éxito en: ${outputFile}!`);

      // Copy to static folder as well
      fs.copyFileSync(outputFile, staticCopy);
      console.log(`✅ Copia guardada en static para descarga web: ${staticCopy}`);
    }
  } catch (err) {
    console.error('❌ Error al exportar PDF con Chrome headless:', err);
  } finally {
    server.close();
    process.exit(0);
  }
});
