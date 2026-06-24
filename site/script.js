document.addEventListener("DOMContentLoaded", () => {
    const observer = new IntersectionObserver((entries) => {
        entries.forEach((entry) => {
            if (entry.isIntersecting) {
                entry.target.classList.add('show');
            }
        });
    }, {
        threshold: 0.1
    });

    const hiddenElements = document.querySelectorAll('.hidden');
    hiddenElements.forEach((el) => observer.observe(el));


    const aboutLink = document.querySelector('a[href="#about"]');
    const drawer = document.getElementById('about-drawer');
    const closeBtn = document.getElementById('drawer-close-btn');
    const closeOverlay = document.getElementById('drawer-close-overlay');

    if (aboutLink && drawer) {
        aboutLink.addEventListener('click', (e) => {
            e.preventDefault();
            drawer.classList.add('active');
            document.body.style.overflow = 'hidden';
        });
    }

    const closeDrawer = () => {
        drawer.classList.remove('active');
        document.body.style.overflow = '';
    };

    if (closeBtn) closeBtn.addEventListener('click', closeDrawer);
    
    if (closeOverlay) closeOverlay.addEventListener('click', closeDrawer);

    window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && drawer.classList.contains('active')) {
            closeDrawer();
        }
    });
});