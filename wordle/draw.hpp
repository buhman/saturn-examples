namespace wordle {
  namespace draw {
    void keyboard(struct screen const& s, void (*draw_char)(uint8_t, int32_t, int32_t, int32_t, int32_t, enum clue));
    void guesses(struct screen const& s, void (*draw_char)(uint8_t, int32_t, int32_t, int32_t, int32_t, enum clue));
  }
}
