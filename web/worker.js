//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   worker.js                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Q9-Kernel läuft komplett in diesem Dedicated Worker (seit Phase 3.6). Grund: OPFS'
//         FileSystemSyncAccessHandle (synchroner Blockzugriff, Pflicht für q9_hal_blk_read/write,
//         das ein synchroner C-Aufruf ist) ist NUR innerhalb eines Workers verfügbar — der
//         Haupt-Thread bleibt für xterm.js/DOM reserviert (E6-Vorgriff: sonst müsste jeder
//         Blockzugriff über einen async postMessage-Umweg laufen, den ein synchroner Syscall
//         nicht abbilden kann).
//         globalThis.q9host  (Konsole, siehe hal_wasm.c)     -> Nachrichten an/von index.html
//         globalThis.q9blk   (Block-Device, siehe hal_wasm.c) -> OPFS-Sync-Access-Handle auf
//         "q9disk.img" im Origin Private File System.
//
// Call:   new Worker('worker.js')  (siehe index.html)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 3.6: Initiale Version — Kernel in den Worker verlagert, OPFS-Blockgerät, │ CF
//         │      │ Image-Upload/-Download (ungetestet, emsdk fehlt lokal)                 │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

const BLK_SIZE     = 512;
const DISK_BLOCKS  = 2880;                             /* 1,44 MB Default-Groesse (Superfloppy)  */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KONSOLE (globalThis.q9host, von hal_wasm.c per EM_JS aufgerufen)                              ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

const inputQueue = [];                                 /* per postMessage('input') gefuellt      */
const utf8 = new TextDecoder('utf-8');                 /* Kernel liefert UTF-8-Bytestrom          */

globalThis.q9host = {
    putc: c => {
        const s = utf8.decode(new Uint8Array([c]), { stream: true });
        if (s) postMessage({ type: 'out', text: s });
    },
    getc: () => inputQueue.length ? inputQueue.shift() : -1
};

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ BLOCK-DEVICE (globalThis.q9blk, OPFS-Sync-Access-Handle)                                     ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

let syncHandle = null;                                 /* erst gesetzt, wenn openImage() fertig  */

globalThis.q9blk = {
    read(lba) {
        if (!syncHandle) return null;
        const buf = new Uint8Array(BLK_SIZE);
        try {
            const n = syncHandle.read(buf, { at: lba * BLK_SIZE });
            if (n < BLK_SIZE) buf.fill(0, n);          /* Lesen ueber EOF hinaus -> Nullbloecke   */
            return buf;
        } catch (e) {
            return null;
        }
    },
    write(lba, data) {
        if (!syncHandle) return false;
        try {
            syncHandle.write(data, { at: lba * BLK_SIZE });
            return true;
        } catch (e) {
            return false;
        }
    }
};

async function openImage() {
    const root = await navigator.storage.getDirectory();
    const fileHandle = await root.getFileHandle('q9disk.img', { create: true });
    syncHandle = await fileHandle.createSyncAccessHandle();
    if (syncHandle.getSize() === 0) {                  /* neues Image -> auf Default-Groesse bringen */
        syncHandle.write(new Uint8Array(BLK_SIZE), { at: (DISK_BLOCKS - 1) * BLK_SIZE });
        syncHandle.flush();
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ NACHRICHTEN VOM HAUPT-THREAD (index.html)                                                    ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

self.onmessage = (ev) => {
    const msg = ev.data;
    switch (msg.type) {
    case 'input':                                      /* Tastendruck                            */
        inputQueue.push(msg.code);
        pump();
        break;
    case 'load-image': {                               /* Upload: Image-Datei ersetzt q9disk.img */
        const bytes = new Uint8Array(msg.data);
        syncHandle.truncate(bytes.length);
        syncHandle.write(bytes, { at: 0 });
        syncHandle.flush();
        postMessage({ type: 'image-loaded', size: bytes.length });
        break;
    }
    case 'get-image': {                                /* Download: aktuelles Image zurueckgeben */
        const size = syncHandle.getSize();
        const buf = new Uint8Array(size);
        syncHandle.read(buf, { at: 0 });
        postMessage({ type: 'image-data', data: buf.buffer }, [buf.buffer]);
        break;
    }
    }
};

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL-ANTRIEB                                                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

let ready = false;
function pump() {
    if (ready) Module._q9_kernel_step();
}

var Module = {
    onRuntimeInitialized() {
        Module._q9_kernel_init();
        ready = true;
        setInterval(pump, 16);                         /* kein rAF im Worker -> Timer reicht      */
    }
};

(async () => {
    await openImage();                                 /* Block-Device muss vor Kernel-Init stehen */
    importScripts('q9.js');
})();

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF worker.js                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
