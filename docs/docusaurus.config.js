// @ts-check
import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'EMS Microgrid DC',
  tagline: 'Documentación del Sistema de Gestión de Energía para Microrredes DC',
  favicon: 'img/favicon.ico',


  url: 'https://ems-microgrid.example.com',
  baseUrl: '/',

  organizationName: 'TobiasLozano',
  projectName: 'ems-visualization',

  onBrokenLinks: 'warn',

  i18n: {
    defaultLocale: 'es',
    locales: ['es'],
  },

  markdown: {
    mermaid: true,
    mdx1Compat: {
      admonitions: true,
    },
  },
  themes: ['@docusaurus/theme-mermaid'],

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          routeBasePath: '/',
          sidebarPath: './sidebars.js',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      image: 'img/resultados/dashboard.png',
      colorMode: {
        defaultMode: 'dark',
        respectPrefersColorScheme: true,
      },
      mermaid: {
        theme: { light: 'neutral', dark: 'dark' },
      },
      navbar: {
        title: 'EMS Microgrid DC',
        items: [
          {
            type: 'docSidebar',
            sidebarId: 'docsSidebar',
            position: 'left',
            label: 'Manual de Usuario',
          },
          {
            to: '/documento-completo',
            label: 'Descargar / Imprimir PDF',
            position: 'right',
            className: 'navbar-download-pdf-btn',
          },
          {
            href: 'https://github.com/TobiasLozano/ems-visualization',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Documentación del Prototipo',
            items: [
              { label: 'Ficha del Prototipo', to: '/' },
              { label: 'Arquitectura Física', to: '/arquitectura-fisica' },
              { label: 'Algoritmos y Control', to: '/funcionamiento-algoritmos' },
              { label: 'Manual Consolidado (PDF)', to: '/documento-completo?print=true' },
            ],
          },
          {
            title: 'Guías y Validación',
            items: [
              { label: 'Modos de Adquisición', to: '/modos-adquisicion' },
              { label: 'Validación Experimental', to: '/validacion-experimental' },
              { label: 'Solución de Problemas', to: '/diagnostico-troubleshooting' },
              { label: 'Repositorio GitHub', href: 'https://github.com/TobiasLozano/ems-visualization' },
            ],
          },
        ],
        copyright: `© ${new Date().getFullYear()} Tobias Lozano — EMS Microgrid DC. Universidad Pontificia Bolivariana.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['bash', 'python', 'arduino', 'json', 'yaml'],
      },
    }),
};

export default config;
