# Clay_Port

Port of Clay to interesting platforms

## Features/Info

- Desktops: 
  - Windowing: GLFW
  - Rendering: OpenGL 1.2
  - Text: libintrafont
  
- Dreamcast: 
  - Windowing: KallistiOS
  - Rendering: GLdc (OpenGL 1.2)
  - Text: libintrafont
    
- Desktops: 
  - Windowing: PSP OS (plain)
  - Rendering: Sony GU
  - Text: libintrafont

## Pre-req

- Meson
- toolchain for intended platform:
  - linux (gcc)
  - dreamcast (KallistiOS)
  - psp (pspsdk)

## Step 1

Configure the project and your platform using meson

Linux:
```bash
meson setup builddir
meson compile -C builddir
```

Dreamcast:
```bash
meson setup --cross-file sh4-dreamcast-kos -Dplatform=dc builddir_dreamcast
meson compile -C builddir_dreamcast
```

Psp:
```bash
meson setup --cross-file mips-allegrex-ps -Dplatform=psp builddir_psp
meson compile -C builddir_psp
```

## Second Level Heading

Paragraph.

- bullet
+ other bullet
* another bullet
    * child bullet

1. ordered
2. next ordered

### Third Level Heading

Some *italic* and **bold** text and `inline code`.

An empty line starts a new paragraph.

Use two spaces at the end  
to force a line break.

A horizontal ruler follows:

---

Add links inline like [this link to the Qt homepage](https://www.qt.io),
or with a reference like [this other link to the Qt homepage][1].

    Add code blocks with
    four spaces at the front.

> A blockquote
> starts with >
>
> and has the same paragraph rules as normal text.

First Level Heading in Alternate Style
======================================

Paragraph.

Second Level Heading in Alternate Style
---------------------------------------

Paragraph.

[1]: https://www.qt.io
