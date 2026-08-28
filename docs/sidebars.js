/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Software',
      items: [
        'software/despliegue-docker',
        'software/extraccion-datos',
      ],
    },
    {
      type: 'category',
      label: 'Firmware',
      items: [
        'firmware/descripcion-general',
        'firmware/parametros-configuracion',
        'firmware/maquina-de-estados',
      ],
    },
    {
      type: 'category',
      label: 'Resultados',
      items: [
        'resultados/analisis-pruebas',
      ],
    },
  ],
};

export default sidebars;
