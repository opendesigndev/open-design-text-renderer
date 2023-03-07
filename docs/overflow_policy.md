## Overflow Policy

Overflow policy describes text visiblity in case it overflows its frame. Note that this **only applicable to frames of 'FIXED' type**.

Overflow policy types in Octopus3+ and 'Text Renderer':

- `NO_OVERFLOW` (Sketch)
- `CLIP_LINE` (XD, Photoshop, Illustrator?)
- `EXTEND_LINE` (might be usefull, e.g. if a different font causes a text to exceed the frame)
- `EXTEND_ALL` (Figma)

### `NO_OVERFLOW`

Text strictly clipped by the frame.

![NO_OVERFLOW]

### `CLIP_LINE`

The last line which doesn't fit the frame is entirely clipped.

![CLIP_LINE]

### `EXTEND_LINE`

Visible text is extended by the last line that at least partially fits the frame.

![EXTEND_LINE]

### `EXTEND_ALL`

The whole text overflow is visible.

![EXTEND_ALL]


[NO_OVERFLOW]: img/NO_OVERFLOW.png "NO_OVERFLOW"
[CLIP_LINE]: img/CLIP_LINE.png "CLIP_LINE"
[EXTEND_LINE]: img/EXTEND_LINE.png "EXTEND_LINE"
[EXTEND_ALL]: img/EXTEND_ALL.png "EXTEND_ALL"
[SET]: img/SET.png "SET"

