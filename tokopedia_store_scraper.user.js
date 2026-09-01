// ==UserScript==
// @name         Tokopedia Floating Minimalist MD Scraper V4
// @namespace    https://tampermonkey.net/
// @version      4.0
// @description  Floating vertical 3-button scraper (Scroll Dasar, Kumpul Data, Save MD) dengan fitur Draggable
// @author       Antigravity
// @match        https://www.tokopedia.com/*
// @grant        GM_setClipboard
// @run-at       document-idle
// ==/UserScript==

(function() {
    'use strict';

    const STORAGE_KEY = 'TKPD_UNIVERSAL_SCRAPED_PRODUCTS';
    let isAutoScrolling = false;

    // Ambil data yang tersimpan dari session
    function getStoredProducts() {
        try {
            const data = sessionStorage.getItem(STORAGE_KEY);
            return data ? new Map(JSON.parse(data)) : new Map();
        } catch (e) {
            return new Map();
        }
    }

    // Simpan data ke session
    function saveProducts(map) {
        try {
            sessionStorage.setItem(STORAGE_KEY, JSON.stringify(Array.from(map.entries())));
        } catch (e) {
            console.error('Gagal menyimpan ke sessionStorage', e);
        }
    }

    function getPageContext() {
        const url = new URL(window.location.href);
        const searchQ = url.searchParams.get('q');
        if (searchQ) return `Pencarian: ${searchQ.replace(/\+/g, ' ')}`;

        const pathParts = url.pathname.split('/').filter(Boolean);
        if (pathParts.length > 0) return `Toko: ${pathParts[0]}`;

        return 'Tokopedia Catalog';
    }

    // Resolusi link produk murni (Handle TopAds r= & direct shop links)
    function resolveProductUrl(rawHref) {
        if (!rawHref) return null;
        try {
            let targetUrl = rawHref;

            // Jika link iklan TopAds (ta.tokopedia.com/promo/v1/clicks?r=...)
            if (targetUrl.includes('ta.tokopedia.com') || targetUrl.includes('/promo/v1/clicks')) {
                const urlObj = new URL(targetUrl, window.location.origin);
                const rParam = urlObj.searchParams.get('r');
                if (rParam) {
                    targetUrl = decodeURIComponent(rParam);
                }
            }

            const parsed = new URL(targetUrl, window.location.origin);
            if (!parsed.hostname.includes('tokopedia.com')) return null;

            const pathParts = parsed.pathname.split('/').filter(Boolean);

            const systemPrefixes = [
                'search', 'discovery', 'find', 'category', 'promo', 'events', 'about', 'help',
                'cart', 'user', 'login', 'register', 'terms', 'privacy', 'blog', 'edu',
                'official-store', 'hot', 'mitra', 'care', 'play', 'stories', 'p', 'people'
            ];

            if (pathParts.length === 2) {
                const shopSlug = pathParts[0].toLowerCase();
                const productSlug = pathParts[1].toLowerCase();

                if (systemPrefixes.includes(shopSlug)) return null;
                if (['product', 'etalase', 'review', 'info', 'feed', 'talk', 'diskusi', 'page'].includes(productSlug)) return null;

                return {
                    cleanUrl: `${parsed.origin}/${pathParts[0]}/${pathParts[1]}`,
                    shop: pathParts[0],
                    slug: pathParts[1]
                };
            }

            return null;
        } catch (e) {
            return null;
        }
    }

    // Ekstrak seluruh produk universal dari DOM saat ini
    function collectProducts(silent = false) {
        const productMap = getStoredProducts();
        let newCount = 0;

        const allLinks = document.querySelectorAll('a[href]');

        allLinks.forEach(a => {
            const rawHref = a.getAttribute('href') || a.href;
            const resolved = resolveProductUrl(rawHref);
            if (!resolved) return;

            const { cleanUrl, shop, slug } = resolved;

            const textNodes = Array.from(a.querySelectorAll('*'))
                .map(el => (el.childNodes.length === 1 && el.childNodes[0].nodeType === 3) ? el.textContent.trim() : '')
                .filter(t => t.length > 0);

            const uniqueTexts = [...new Set(textNodes)];

            // 1. Ekstrak Harga
            const priceItem = uniqueTexts.find(t => /^Rp\s?[\d\.]+/i.test(t)) || '-';

            // 2. Ekstrak Terjual
            const soldItem = uniqueTexts.find(t => /\bterjual\b/i.test(t)) || '-';

            // 3. Ekstrak Rating
            const ratingItem = uniqueTexts.find(t => /(?:⭐|\b[1-5]\.\d\b)/.test(t)) || '-';

            // 4. Ekstrak Toko / Lokasi
            const metaCandidates = uniqueTexts.filter(t => 
                !t.startsWith('Rp') &&
                !t.includes('%') &&
                !/terjual/i.test(t) &&
                !/hemat/i.test(t) &&
                !/cashback/i.test(t) &&
                !/gopay/i.test(t) &&
                !/cicil/i.test(t) &&
                !/ulasan/i.test(t) &&
                t.length >= 3 && t.length <= 30
            );

            let shopName = shop;
            if (metaCandidates.length > 1) {
                shopName = metaCandidates[metaCandidates.length - 1];
            }

            // 5. Ekstrak Nama Produk
            const titleCandidates = uniqueTexts.filter(t => 
                !t.startsWith('Rp') &&
                !t.includes('%') &&
                !/terjual/i.test(t) &&
                !/hemat/i.test(t) &&
                !/cashback/i.test(t) &&
                !/gopay/i.test(t) &&
                !/cicil/i.test(t) &&
                !/ulasan/i.test(t) &&
                t.length > 8
            );

            let title = titleCandidates.sort((x, y) => y.length - x.length)[0] || 
                        a.getAttribute('title') || 
                        slug.replace(/-/g, ' ');

            if (title && cleanUrl) {
                if (!productMap.has(cleanUrl)) {
                    productMap.set(cleanUrl, {
                        name: title.replace(/[\r\n\t]+/g, ' ').trim(),
                        price: priceItem,
                        shop: shopName.trim(),
                        sold: soldItem,
                        rating: ratingItem,
                        url: cleanUrl
                    });
                    newCount++;
                }
            }
        });

        if (newCount > 0) {
            saveProducts(productMap);
        }
        updateBadge(productMap.size);

        if (!silent) {
            showToast(`⚡ Berhasil mengumpulkan ${productMap.size} produk! (+${newCount} baru)`);
        }
        return productMap.size;
    }

    // Tombol 1: Scroll Sampai Dasar Halaman
    async function scrollToBottom() {
        const btnScroll = document.getElementById('tkpd-fab-scroll');

        if (isAutoScrolling) {
            isAutoScrolling = false;
            if (btnScroll) btnScroll.style.background = '#1e293b';
            showToast('⏹️ Scroll dihentikan');
            return;
        }

        isAutoScrolling = true;
        if (btnScroll) {
            btnScroll.style.background = '#0284c7';
            btnScroll.classList.add('tkpd-pulsing');
        }

        showToast('⬇️ Sedang scroll sampai dasar...');

        let lastScrollHeight = 0;
        let unchangedCount = 0;
        const maxUnchanged = 4;

        while (isAutoScrolling && unchangedCount < maxUnchanged) {
            const currentHeight = document.body.scrollHeight;
            const currentY = window.scrollY;
            const viewportH = window.innerHeight;

            // Scroll bertahap
            window.scrollBy({ top: 750, behavior: 'smooth' });
            await new Promise(r => setTimeout(r, 450));

            // Jika posisi mendekati dasar halaman
            if (currentY + viewportH >= currentHeight - 350) {
                await new Promise(r => setTimeout(r, 1100));
                const newHeight = document.body.scrollHeight;
                if (newHeight <= currentHeight) {
                    unchangedCount++;
                    window.scrollBy(0, -300);
                    await new Promise(r => setTimeout(r, 350));
                    window.scrollTo(0, document.body.scrollHeight);
                    await new Promise(r => setTimeout(r, 900));
                } else {
                    unchangedCount = 0;
                }
                lastScrollHeight = newHeight;
            } else {
                unchangedCount = 0;
            }
        }

        isAutoScrolling = false;
        if (btnScroll) {
            btnScroll.style.background = '#1e293b';
            btnScroll.classList.remove('tkpd-pulsing');
        }

        // Scroll selesai, berikan notifikasi agar user menekan tombol petir
        showToast('🏁 Sampai dasar! Klik ⚡ untuk kumpulkan data.');
    }

    // Tombol 3: Download Markdown (.md)
    function downloadMarkdown() {
        const productMap = getStoredProducts();
        if (productMap.size === 0) {
            // Coba kumpulkan sekali jika belum
            collectProducts(true);
        }

        const freshMap = getStoredProducts();
        if (freshMap.size === 0) {
            showToast('⚠️ Belum ada produk. Klik ⚡ dulu!');
            return;
        }

        const contextTitle = getPageContext();
        const now = new Date().toLocaleString('id-ID', { dateStyle: 'full', timeStyle: 'medium' });
        const items = Array.from(freshMap.values());

        let md = `# 📦 Katalog Produk: ${contextTitle}\n\n`;
        md += `- **URL Sumber**: [${window.location.href}](${window.location.href})\n`;
        md += `- **Total Produk**: ${items.length} item\n`;
        md += `- **Waktu Ekstraksi**: ${now}\n\n`;
        md += `---\n\n`;

        md += `## 📊 Tabel Ringkasan Produk\n\n`;
        md += `| No | Nama Produk | Harga | Toko / Info | Terjual | Rating | Link Produk |\n`;
        md += `|:---:|:---|:---:|:---:|:---:|:---:|:---:|\n`;

        items.forEach((item, idx) => {
            const safeTitle = item.name.replace(/\|/g, '-');
            const safeShop = (item.shop || '-').replace(/\|/g, '-');
            md += `| ${idx + 1} | ${safeTitle} | ${item.price} | ${safeShop} | ${item.sold} | ${item.rating} | [Buka Produk](${item.url}) |\n`;
        });

        md += `\n---\n\n`;
        md += `## 📝 Daftar Tautan Lengkap (List View)\n\n`;

        items.forEach((item, idx) => {
            md += `${idx + 1}. **[${item.name}](${item.url})**\n`;
            md += `   - **Harga**: ${item.price}\n`;
            md += `   - **Toko/Info**: ${item.shop}\n`;
            md += `   - **Status**: ${item.sold} • ${item.rating}\n`;
            md += `   - **URL**: \`${item.url}\`\n\n`;
        });

        const url = new URL(window.location.href);
        const searchQ = url.searchParams.get('q');
        const pathParts = url.pathname.split('/').filter(Boolean);
        const namePart = searchQ ? `search_${searchQ.replace(/[^a-zA-Z0-9]/g, '_')}` : (pathParts[0] || 'tokopedia');

        const blob = new Blob([md], { type: 'text/markdown;charset=utf-8;' });
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = `katalog_${namePart}.md`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);

        showToast(`💾 Tersimpan: katalog_${namePart}.md`);
    }

    function updateBadge(count) {
        const badge = document.getElementById('tkpd-fab-badge');
        if (badge) {
            badge.innerText = count;
            badge.style.display = count > 0 ? 'block' : 'none';
        }
    }

    // Toast Notification Ringan
    function showToast(text) {
        let toast = document.getElementById('tkpd-toast');
        if (!toast) {
            toast = document.createElement('div');
            toast.id = 'tkpd-toast';
            toast.style.cssText = `
                position: fixed !important;
                bottom: 24px !important;
                left: 90px !important;
                background: #0f172a !important;
                color: #38bdf8 !important;
                border: 1px solid #334155 !important;
                padding: 8px 16px !important;
                border-radius: 999px !important;
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif !important;
                font-size: 12px !important;
                font-weight: 600 !important;
                box-shadow: 0 10px 20px rgba(0,0,0,0.5) !important;
                z-index: 2147483647 !important;
                pointer-events: none !important;
                transition: opacity 0.3s, transform 0.3s !important;
                opacity: 0;
                transform: translateY(10px);
            `;
            document.documentElement.appendChild(toast);
        }

        toast.innerText = text;
        toast.style.opacity = '1';
        toast.style.transform = 'translateY(0)';

        clearTimeout(toast.hideTimeout);
        toast.hideTimeout = setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateY(10px)';
        }, 2500);
    }

    // Injeksi Floating Action Bar Vertikal + Draggable
    function injectFloatingBar() {
        if (document.getElementById('tkpd-fab-container')) return;

        // Injeksi CSS Animasi & Tooltip
        const style = document.createElement('style');
        style.innerHTML = `
            @keyframes tkpdPulse {
                0% { transform: scale(1); box-shadow: 0 0 0 0 rgba(56, 189, 248, 0.7); }
                70% { transform: scale(1.08); box-shadow: 0 0 0 10px rgba(56, 189, 248, 0); }
                100% { transform: scale(1); box-shadow: 0 0 0 0 rgba(56, 189, 248, 0); }
            }
            .tkpd-pulsing {
                animation: tkpdPulse 1.5s infinite !important;
            }
            .tkpd-btn-hover:hover {
                transform: scale(1.1) !important;
                border-color: #38bdf8 !important;
            }
            .tkpd-btn-hover:active {
                transform: scale(0.95) !important;
            }
        `;
        document.head.appendChild(style);

        const container = document.createElement('div');
        container.id = 'tkpd-fab-container';
        container.style.cssText = `
            position: fixed !important;
            bottom: 24px !important;
            left: 24px !important;
            display: flex !important;
            flex-direction: column !important;
            gap: 10px !important;
            background: rgba(15, 23, 42, 0.85) !important;
            backdrop-filter: blur(8px) !important;
            border: 1.5px solid rgba(56, 189, 248, 0.3) !important;
            border-radius: 30px !important;
            padding: 8px 6px !important;
            box-shadow: 0 15px 30px rgba(0, 0, 0, 0.6) !important;
            z-index: 2147483647 !important;
            user-select: none !important;
            touch-action: none !important;
            cursor: grab !important;
        `;

        container.innerHTML = `
            <!-- Drag Handle Bar Kecil di Atas -->
            <div id="tkpd-drag-handle" style="
                width: 24px;
                height: 4px;
                background: #475569;
                border-radius: 999px;
                margin: 2px auto 4px auto;
                cursor: grab;
            " title="Geser posisi"></div>

            <!-- Tombol 1: Panah ke Bawah (Scroll Dasar) -->
            <button id="tkpd-fab-scroll" class="tkpd-btn-hover" style="
                position: relative;
                width: 44px;
                height: 44px;
                background: #1e293b;
                border: 1.5px solid #334155;
                border-radius: 50%;
                color: #38bdf8;
                font-size: 20px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
                outline: none;
            " title="⬇️ Scroll Sampai Dasar Halaman">
                ⬇️
            </button>

            <!-- Tombol 2: Petir (Kumpulkan Data) -->
            <button id="tkpd-fab-collect" class="tkpd-btn-hover" style="
                position: relative;
                width: 44px;
                height: 44px;
                background: #1e293b;
                border: 1.5px solid #334155;
                border-radius: 50%;
                color: #fbbf24;
                font-size: 20px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
                outline: none;
            " title="⚡ Kumpulkan Data Produk yang Tampil">
                ⚡
                <!-- Badge Total Produk -->
                <span id="tkpd-fab-badge" style="
                    display: none;
                    position: absolute;
                    top: -4px;
                    right: -4px;
                    background: #ef4444;
                    color: white;
                    font-family: -apple-system, BlinkMacSystemFont, sans-serif;
                    font-size: 10px;
                    font-weight: 800;
                    padding: 1px 5px;
                    border-radius: 999px;
                    border: 1.5px solid #0f172a;
                ">0</span>
            </button>

            <!-- Tombol 3: Floppy Disk / Disket (Save Markdown) -->
            <button id="tkpd-fab-save" class="tkpd-btn-hover" style="
                width: 44px;
                height: 44px;
                background: #1e293b;
                border: 1.5px solid #334155;
                border-radius: 50%;
                color: #10b981;
                font-size: 20px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
                outline: none;
            " title="💾 Unduh File Markdown (.md)">
                💾
            </button>
        `;

        document.documentElement.appendChild(container);

        // Pasang Event Listener Klik
        document.getElementById('tkpd-fab-scroll').addEventListener('click', scrollToBottom);
        document.getElementById('tkpd-fab-collect').addEventListener('click', () => collectProducts(false));
        document.getElementById('tkpd-fab-save').addEventListener('click', downloadMarkdown);

        // Update badge awal
        updateBadge(getStoredProducts().size);

        // --- Implementasi Fitur Drag & Drop Bebas ---
        let isDragging = false;
        let startX, startY, initialLeft, initialTop;

        container.addEventListener('mousedown', (e) => {
            if (e.target.tagName.toLowerCase() === 'button') return; // Jangan drag saat klik button
            isDragging = true;
            container.style.cursor = 'grabbing';
            startX = e.clientX;
            startY = e.clientY;
            
            const rect = container.getBoundingClientRect();
            initialLeft = rect.left;
            initialTop = rect.top;
            
            // Ubah bottom/left styling ke fixed top/left saat mulai drag
            container.style.bottom = 'auto';
            container.style.left = `${initialLeft}px`;
            container.style.top = `${initialTop}px`;
        });

        document.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            const dx = e.clientX - startX;
            const dy = e.clientY - startY;
            
            let newX = initialLeft + dx;
            let newY = initialTop + dy;

            // Batasi agar tidak keluar layar
            newX = Math.max(10, Math.min(window.innerWidth - 65, newX));
            newY = Math.max(10, Math.min(window.innerHeight - 170, newY));

            container.style.left = `${newX}px`;
            container.style.top = `${newY}px`;
        });

        document.addEventListener('mouseup', () => {
            if (isDragging) {
                isDragging = false;
                container.style.cursor = 'grab';
            }
        });
    }

    // Inisialisasi widget
    setInterval(() => {
        if (!document.getElementById('tkpd-fab-container')) {
            injectFloatingBar();
        }
    }, 1000);

})();
