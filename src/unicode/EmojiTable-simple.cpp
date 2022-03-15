#include "EmojiTable.h"

#ifndef FULL_EMOJI_TABLE

namespace textify {
namespace unicode {

EmojiTable EmojiTable::create()
{
    EmojiTable inst;

    inst.add(0x0023);
    inst.add(0x002A);
    inst.add(0x0030,0x0039);
    inst.add(0x00A9);
    inst.add(0x00AE);
    inst.add(0x203C);
    inst.add(0x2049);
    inst.add(0x2122);
    inst.add(0x2139);
    inst.add(0x2194,0x2199);
    inst.add(0x21A9,0x21AA);
    inst.add(0x231A,0x231B);
    inst.add(0x2328);
    inst.add(0x23CF);
    inst.add(0x23E9,0x23EC);
    inst.add(0x23ED,0x23EE);
    inst.add(0x23EF);

    /*
0023          ; Emoji                # E0.0   [1] (#️)       number sign
002A          ; Emoji                # E0.0   [1] (*️)       asterisk
0030..0039    ; Emoji                # E0.0  [10] (0️..9️)    digit zero..digit nine
00A9          ; Emoji                # E0.6   [1] (©️)       copyright
00AE          ; Emoji                # E0.6   [1] (®️)       registered
203C          ; Emoji                # E0.6   [1] (‼️)       double exclamation mark
2049          ; Emoji                # E0.6   [1] (⁉️)       exclamation question mark
2122          ; Emoji                # E0.6   [1] (™️)       trade mark
2139          ; Emoji                # E0.6   [1] (ℹ️)       information
2194..2199    ; Emoji                # E0.6   [6] (↔️..↙️)    left-right arrow..down-left arrow
21A9..21AA    ; Emoji                # E0.6   [2] (↩️..↪️)    right arrow curving left..left arrow curving right
231A..231B    ; Emoji                # E0.6   [2] (⌚..⌛)    watch..hourglass done
2328          ; Emoji                # E1.0   [1] (⌨️)       keyboard
23CF          ; Emoji                # E1.0   [1] (⏏️)       eject button
23E9..23EC    ; Emoji                # E0.6   [4] (⏩..⏬)    fast-forward button..fast down button
23ED..23EE    ; Emoji                # E0.7   [2] (⏭️..⏮️)    next track button..last track button
23EF
*/

    return inst;
}

} // namespace unicode
} // namespace textify
#endif
