/* Embed the raw PCM clip (mono, 16 kHz, 16-bit LE) directly into flash rodata.
   Avoids a multi-megabyte C array + slow compile. */
    .section .rodata.audio
    .global audio_pcm
    .global audio_pcm_end
    .align 4
audio_pcm:
    .incbin "/Users/malin/Documents/hackathon-axiometa/audio_play/clip.raw"
audio_pcm_end:
    .align 4
