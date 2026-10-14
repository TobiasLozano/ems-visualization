import React from 'react';

/**
 * Academic & Publication-Grade Figure Component
 * Designed specifically for clean document organization and PDF export.
 * Replaces interactive widgets with clean, printable semantics (<figure>, <figcaption>).
 */
export default function Figure({
  src,
  title,
  figureNumber,
  description,
  alt,
  width = '100%',
  maxWidth = '920px'
}) {
  const figureLabel = figureNumber ? `Figura ${figureNumber}` : null;
  const altText = alt || title || (description ? `${figureLabel || 'Figura'}: ${description}` : 'Diagrama');

  return (
    <figure
      className="academic-figure"
      style={{
        margin: '28px auto',
        textAlign: 'center',
        width: width,
        maxWidth: maxWidth
      }}
    >
      <div className="academic-figure-image-wrapper">
        <img
          src={src}
          alt={altText}
          loading="lazy"
          className="academic-figure-img"
        />
      </div>

      {(figureNumber || title || description) && (
        <figcaption className="academic-figure-caption">
          {figureNumber && <strong className="figure-num">Figura {figureNumber}: </strong>}
          {title && <span className="figure-title">{title}. </span>}
          {description && <span className="figure-desc">{description}</span>}
        </figcaption>
      )}
    </figure>
  );
}
