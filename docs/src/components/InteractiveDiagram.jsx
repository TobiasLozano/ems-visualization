import React from 'react';
import Figure from './Figure';

/**
 * Backward-compatible wrapper for InteractiveDiagram.
 * Redirects to the clean academic Figure component to ensure
 * clean document organization and PDF compatibility without interactive clutter.
 */
export default function InteractiveDiagram(props) {
  return <Figure {...props} />;
}
