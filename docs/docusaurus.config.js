// @ts-check
import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'EMS Microgrid DC',
  tagline: 'Documentación del Sistema de Gestión de Energía para Microrredes DC',
  favicon: 'img/favicon.ico',

  future: {
    v4: true,
  },

  url: 'https://ems-microgrid.example.com',
  baseUrl: '/',

  organizationName: 'TobiasLozano',
  projectName: 'ems-visualization',

  onBrokenLinks: 'throw',

  i18n: {
    defaultLocale: 'es',
    locales: ['es'],
  },

  markdown: {
    mermaid: true,
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
            label: 'Documentación',
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
            title: 'Documentación',
            items: [
              { label: 'Arquitectura', to: '/' },
              { label: 'Firmware', to: '/firmware/descripcion-general' },
              { label: 'Resultados', to: '/resultados/analisis-pruebas' },
            ],
          },
          {
            title: 'Herramientas',
            items: [
              { label: 'Grafana', href: 'http://localhost:3000' },
              { label: 'Web App', href: 'http://localhost:5000' },
            ],
          },
        ],
        copyright: `© ${new Date().getFullYear()} Tobias Lozano — EMS Microgrid DC. Built with Docusaurus.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['bash', 'python', 'arduino', 'json', 'yaml'],
      },
    }),
};

export default config;
