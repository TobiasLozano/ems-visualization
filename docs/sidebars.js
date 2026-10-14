/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docsSidebar: [
    {
      type: 'doc',
      id: 'intro',
      label: '1. Identificación del Prototipo',
    },
    {
      type: 'doc',
      id: 'modos-adquisicion',
      label: '2. Modos de Adquisición y Hardware',
    },
    {
      type: 'doc',
      id: 'entornos-ejecucion',
      label: '3. Entornos de Ejecución',
    },
    {
      type: 'doc',
      id: 'monitoreo-grafana',
      label: '4. Monitoreo en Grafana y Panel Web',
    },
    {
      type: 'doc',
      id: 'validacion-experimental',
      label: '5. Validación Experimental',
    },
    {
      type: 'doc',
      id: 'diagnostico-troubleshooting',
      label: '6. Solución de Problemas',
    },
    {
      type: 'category',
      label: 'Anexos de Referencia Técnica',
      collapsed: true,
      items: [
        'anexo-referencia-tecnica',
        'descripcion-general',
        'arquitectura-fisica',
        'componentes-hardware',
        'funcionamiento-algoritmos',
        'arquitectura-software',
      ],
    },
  ],
};

export default sidebars;
