# Local guest images

`flux-x86.img` is the local OS-9000 U4.9 XiBase reference image.  It is
proprietary and ignored by Git.  The default `profiles/os9000-4.9.conf` refers
to it; `bin/flux-x86` runs it with QEMU snapshot mode by default, so a normal
boot does not alter the file.

Do not commit guest images here.  Add a documented profile for each additional
locally held image instead.

## XiBase9 graphical desktop

After the OS-9000 text-mode boot reaches the `/dd/>` prompt, start the bundled
XiBase9 graphical environment with:

```text
xb
```

followed by Enter.  This was verified on 2026-09-03 with
`bin/flux-x86 --config profiles/os9000-4.9.conf`: the guest switched from the
720x400 text console to the 1024x768 XiBase9 desktop.  The desktop provides a
Start menu, File Manager and Terminal Program.
