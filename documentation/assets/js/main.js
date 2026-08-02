document.addEventListener('DOMContentLoaded', () => {
  const tocLinks = document.querySelectorAll('#TableOfContents a[href]');

  tocLinks.forEach(tocLink => {
    tocLink.addEventListener('click', event => {
      const targetElementId = tocLink.getAttribute('href').substring(1);
      const targetElement = document.getElementById(targetElementId);

      if (targetElement) {
        targetElement.click();
        // Find the closest <details> ancestor
        /*const detailsElement = targetElement.closest('details');

        if (detailsElement) {
          event.preventDefault();
          detailsElement.open = true;

          // Scroll the <summary> element into view
          const summaryElement = detailsElement.querySelector('summary');
          if (summaryElement) {
            summaryElement.scrollIntoView({ behavior: 'smooth' });
          } else {
            // Fallback to scrolling the <details> element
            detailsElement.scrollIntoView({ behavior: 'smooth' });
          }
        }*/
      }
    });
  });
});
