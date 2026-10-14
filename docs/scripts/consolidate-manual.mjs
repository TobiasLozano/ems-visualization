import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const docsDir = path.resolve(__dirname, '../docs');
const targetFile = path.resolve(docsDir, 'documento-completo.md');

const chapterFiles = [
  'intro.md',
  '01-modos-adquisicion.md',
  '02-entornos-ejecucion.md',
  '03-monitoreo-grafana.md',
  '04-validacion-experimental.md',
  '05-diagnostico-troubleshooting.md'
];

function extractBody(content, isIntro = false) {
  // Strip YAML frontmatter
  let body = content.replace(/^---[\s\S]*?---\r?\n/, '');

  // Strip imports
  body = body.replace(/import\s+.*?;?\r?\n/g, '');

  if (isIntro) {
    // Remove the PDF download card from the intro when inside the PDF view itself
    body = body.replace(/<div className="pdf-download-card">[\s\S]*?<\/a>\s*<\/div>\s*<\/div>/g, '');
  }

  return body.trim();
}

console.log('🔄 Consolidando capítulos del Manual de Usuario desde archivos markdown...');

const bodies = [];

for (const file of chapterFiles) {
  const filePath = path.join(docsDir, file);
  if (!fs.existsSync(filePath)) {
    console.warn(`⚠️ Archivo de capítulo no encontrado: ${filePath}`);
    continue;
  }
  const raw = fs.readFileSync(filePath, 'utf-8');
  const body = extractBody(raw, file === 'intro.md');
  bodies.push(body);
}

const header = `---
slug: /documento-completo
title: Manual Consolidado (PDF)
displayed_sidebar: null
---

import Figure from '@site/src/components/Figure';

<div className="manual-pdf-header" style={{ textAlign: 'center', padding: '30px 20px', borderBottom: '2px solid var(--ifm-color-primary)', marginBottom: '32px' }}>
  <div style={{ fontSize: '1.2rem', fontWeight: 800, color: 'var(--ifm-color-primary)', textTransform: 'uppercase', letterSpacing: '1px' }}>
    Manual de Usuario y Operación del Software
  </div>
  <h1 style={{ margin: '12px 0 6px 0', fontSize: '2.2rem' }}>
    Sistema EMS para Microrredes en Corriente Continua
  </h1>
  <div style={{ fontSize: '1rem', color: 'var(--ifm-color-content-secondary)' }}>
    Control Fotovoltaico MPPT · Carga Segura de Baterías CC/CV · Protección Térmica Activa · Monitoreo Web y Grafana
  </div>
  <div style={{ marginTop: '18px', display: 'flex', justifyContent: 'center', gap: '14px', flexWrap: 'wrap' }}>
    <button
      className="button button--primary button--lg"
      style={{ padding: '12px 28px', fontSize: '1.05rem', fontWeight: 'bold', boxShadow: '0 4px 14px rgba(0,0,0,0.15)', cursor: 'pointer' }}
      onClick={() => typeof window !== 'undefined' && window.print()}
    >
      Imprimir / Guardar Manual como PDF
    </button>
  </div>
</div>
`;

const separator = '\n\n';

const fullDoc = header + '\n\n' + bodies.join(separator) + '\n';

fs.writeFileSync(targetFile, fullDoc, 'utf-8');
console.log(`✅ Manual consolidado generado exitosamente en: ${targetFile}`);
console.log(`📄 Total de capítulos consolidados: ${bodies.length}`);
