
Tests

int main() {
    
    text_renderer_init();

    int x = 0, y = 0;
    for (int i = 0; i < FONT_COUNT; i++) {
        /
        if (y > (EPD_HEIGHT - 7)) break;

        draw_char(x, y, font5x7[i].c);

        x += 8;
        if (x > 120) {
            x = 0;
            y += 8;
        }
    }
    draw_logo();

    for (int yy = 0; yy < Y_LINES; yy++) {
        for (int xb = 0; xb < X_BYTES; xb++) {
            size_t idx = yy * X_BYTES + xb;
            if (idx >= X_BYTES * Y_LINES) {
                fprintf(stderr, "Out of bounds index %zu\n", idx);
                return 1;
            }
            uint8_t byte = framebuf[idx];
            for (int bit = 7; bit >= 0; bit--) {
                putchar((byte & (1 << bit)) ? ' ' : 'x');
            }
        }
        putchar('\n');
    }

    return 0;
}