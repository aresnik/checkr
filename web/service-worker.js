// 1. Increment this version name EVERY time you update the game files (e.g., checkr-v3, checkr-v4)
const CACHE_NAME = 'checkr-v3';

const ASSETS = [
    './',
    './index.html',
    './index.js',
    './index.wasm', // Your compiled C++ code
    './index.data', // Your SDL3 images/assets package
    './manifest.json',
    './icon.png'
];

// Install Event: Cache the new assets
self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME).then((cache) => {
            return cache.addAll(ASSETS);
        })
    );
    // Forces the waiting service worker to become active immediately
    self.skipWaiting();
});

// Activate Event: Clear out all OLD cache folders automatically
self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((cacheNames) => {
            return Promise.all(
                cacheNames.map((cache) => {
                    if (cache !== CACHE_NAME) {
                        console.log('Clearing old PWA cache:', cache);
                        return caches.delete(cache); // Deletes old versions automatically
                    }
                })
            );
        })
    );
    // Forces the service worker to take control of the open page immediately
    self.clients.claim();
});