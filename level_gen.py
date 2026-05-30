import pygame
import sys

# =========================
# 常量设置
# =========================
WIDTH = 40
HEIGHT = 40
CELL_SIZE = 20
MARGIN = 0

SCREEN_WIDTH = WIDTH * (CELL_SIZE + MARGIN) + MARGIN
SCREEN_HEIGHT = HEIGHT * (CELL_SIZE + MARGIN) + MARGIN

SAVE_FILE = "level.txt"

COLORS = [
    (50, 50, 50),       # 0
    (80, 120, 200),     # 1
    (80, 180, 120),     # 2
    (220, 180, 80),     # 3
    (220, 100, 80),     # 4
    (160, 100, 220),    # 5
    (80, 200, 200),     # 6
    (230, 120, 180),    # 7
    (150, 150, 150),    # 8
    (255, 255, 120),    # 9
    (255, 255, 255),    # 10
]


def save_level(grid):
    with open(SAVE_FILE, "w", encoding="utf-8") as f:
        for y in range(HEIGHT):
            f.write(" ".join(str(grid[y][x]) for x in range(WIDTH)) + "\n")
    print("saved to", SAVE_FILE)


def load_level(grid):
    try:
        with open(SAVE_FILE, "r", encoding="utf-8") as f:
            lines = f.readlines()

        for y in range(min(HEIGHT, len(lines))):
            nums = lines[y].strip().split()
            for x in range(min(WIDTH, len(nums))):
                value = int(nums[x])
                if 0 <= value <= 10:
                    grid[y][x] = value

        print("loaded from", SAVE_FILE)
    except FileNotFoundError:
        print("level.txt not found")


def get_cell_from_mouse(mx, my):
    x = (mx - MARGIN) // (CELL_SIZE + MARGIN)
    y = (my - MARGIN) // (CELL_SIZE + MARGIN)

    # 鼠标点在 margin 缝隙里时，不算命中格子
    local_x = (mx - MARGIN) % (CELL_SIZE + MARGIN)
    local_y = (my - MARGIN) % (CELL_SIZE + MARGIN)

    if local_x >= CELL_SIZE or local_y >= CELL_SIZE:
        return None, None

    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        return x, y

    return None, None


def change_cell(grid, x, y, delta):
    grid[y][x] = (grid[y][x] + delta) % 11
    print("change:", x, y, "value:", grid[y][x])


pygame.init()
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
pygame.display.set_caption("简单关卡编辑器")

font = None
try:
    pygame.font.init()
    font = pygame.font.SysFont("arial", 20)
except Exception:
    font = None

grid = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

mouse_down = False
paint_delta = 0
last_cell = None

running = True
while running:
    screen.fill((25, 25, 25))

    for y in range(HEIGHT):
        for x in range(WIDTH):
            value = grid[y][x]

            rect = pygame.Rect(
                MARGIN + x * (CELL_SIZE + MARGIN),
                MARGIN + y * (CELL_SIZE + MARGIN),
                CELL_SIZE,
                CELL_SIZE
            )

            pygame.draw.rect(screen, COLORS[value], rect)
            pygame.draw.rect(screen, (120, 120, 120), rect, 1)

            if font is not None:
                text_color = (0, 0, 0) if value >= 9 else (255, 255, 255)
                text = font.render(str(value), True, text_color)
                text_rect = text.get_rect(center=rect.center)
                screen.blit(text, text_rect)

    pygame.display.flip()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                running = False
            elif event.key == pygame.K_s:
                save_level(grid)
            elif event.key == pygame.K_l:
                load_level(grid)

        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                mouse_down = True
                paint_delta = 1
                last_cell = None
            elif event.button == 3:
                mouse_down = True
                paint_delta = -1
                last_cell = None

            if event.button in (1, 3):
                x, y = get_cell_from_mouse(*event.pos)
                if x is not None:
                    change_cell(grid, x, y, paint_delta)
                    last_cell = (x, y)

        elif event.type == pygame.MOUSEBUTTONUP:
            if event.button in (1, 3):
                mouse_down = False
                paint_delta = 0
                last_cell = None

        elif event.type == pygame.MOUSEMOTION:
            if mouse_down:
                x, y = get_cell_from_mouse(*event.pos)
                if x is not None and (x, y) != last_cell:
                    change_cell(grid, x, y, paint_delta)
                    last_cell = (x, y)

pygame.quit()
sys.exit()