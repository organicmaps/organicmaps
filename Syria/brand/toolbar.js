// Download toolbar for the brand pages: "download as image / PDF" buttons for the design shown.
// The page lists its files on <body data-images="a.png,b.png" data-labels="..." data-pdf="x.pdf" data-home="index.html">,
// paths relative to the page. Hidden when printing and in exports (?export or ?page=N), so it never ends up in a design.
(function () {
  const params = new URLSearchParams(location.search);
  if (params.has('export') || params.has('page')) return;
  const d = document.body.dataset;
  const images = (d.images || '').split(',').filter(Boolean);
  const labels = (d.labels || '').split(',');

  const css = document.createElement('style');
  css.textContent = `
    .dl-bar { position: fixed; left: 16px; bottom: 16px; z-index: 1000; display: flex; flex-wrap: wrap; gap: 8px;
              max-width: calc(100vw - 32px); padding: 8px; border-radius: 16px; background: rgba(33,36,42,.92);
              box-shadow: 0 10px 30px -10px rgba(0,0,0,.5); font: 600 13px/1 "Readex Pro", "IBM Plex Sans Arabic", sans-serif; direction: rtl; }
    .dl-bar a { display: inline-flex; align-items: center; gap: 6px; padding: 10px 13px; border-radius: 10px; text-decoration: none;
                color: #fff; background: #404E5E; white-space: nowrap; }
    .dl-bar a.main { background: #0C7C70; }
    .dl-bar a:hover { filter: brightness(1.15); }
    .dl-bar svg { width: 16px; height: 16px; }
    @media print { .dl-bar { display: none !important; } }`;
  document.head.appendChild(css);

  const icon = (p) => `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true">${p}</svg>`;
  const IMG = icon('<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 16l5-5 4 4 3-3 6 6"/>');
  const PDF = icon('<path d="M6 2h9l5 5v15H6z"/><path d="M14 2v6h6"/>');
  const HOME = icon('<path d="M3 11l9-8 9 8v10H3z"/>');

  const bar = document.createElement('nav');
  bar.className = 'dl-bar';
  bar.setAttribute('aria-label', 'تنزيل');
  const link = (href, html, cls, download) => {
    const a = document.createElement('a');
    a.href = href;
    a.innerHTML = html;
    if (cls) a.className = cls;
    if (download) a.setAttribute('download', '');
    bar.appendChild(a);
  };
  images.forEach((src, i) => {
    const label = labels[i] || (images.length > 1 ? `صورة ${i + 1}` : '');
    link(src, `${IMG} تنزيل كصورة${label ? ' · ' + label : ''}`, 'main', true);
  });
  if (d.pdf) link(d.pdf, `${PDF} تنزيل PDF`, '', true);
  if (d.home) link(d.home, `${HOME} كل التصاميم`, '', false);
  document.body.appendChild(bar);
})();
