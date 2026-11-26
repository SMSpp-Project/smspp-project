// Show the caption of each figure on the site. A figure of the Markdown is
// `![Figure N.M. caption](../figures/x.svg){#fig-N-M}`: Pandoc makes it a
// LaTeX figure with that caption, while MkDocs leaves the caption only in
// the alt text of the image. Here each image whose id starts with "fig-" is
// wrapped in a <figure> with the alt text as its <figcaption>, before
// katex-init.js (loaded after this file) typesets the math in it.
document$.subscribe(() => {
  document.querySelectorAll('img[id^="fig-"]').forEach((img) => {
    if (img.closest("figure")) return;
    const fig = document.createElement("figure");
    const cap = document.createElement("figcaption");
    // the text of the alt, with \textit{x} out of the math set as x in
    // italics, as LaTeX sets it in the PDF
    const alt = img.getAttribute("alt") || "";
    alt.split(/(\\\(.*?\\\))/).forEach((part, i) => {
      if (i % 2) { cap.append(part); return; }
      part.split(/\\textit\{([^{}]*)\}/).forEach((t, j) => {
        if (j % 2) {
          const em = document.createElement("em");
          em.textContent = t;
          cap.append(em);
        } else {
          cap.append(t);
        }
      });
    });
    img.parentNode.insertBefore(fig, img);
    fig.appendChild(img);
    fig.appendChild(cap);
  });
});
