from pathlib import Path

import imageio.v3 as iio
import numpy as np


def _border_connected_mask(mask: np.ndarray) -> np.ndarray:
    height, width = mask.shape
    connected = np.zeros_like(mask, dtype=bool)
    stack: list[tuple[int, int]] = []

    for x in range(width):
        if mask[0, x]:
            stack.append((0, x))
        if mask[height - 1, x]:
            stack.append((height - 1, x))
    for y in range(height):
        if mask[y, 0]:
            stack.append((y, 0))
        if mask[y, width - 1]:
            stack.append((y, width - 1))

    while stack:
        y, x = stack.pop()
        if connected[y, x] or not mask[y, x]:
            continue
        connected[y, x] = True

        if y > 0:
            stack.append((y - 1, x))
        if y + 1 < height:
            stack.append((y + 1, x))
        if x > 0:
            stack.append((y, x - 1))
        if x + 1 < width:
            stack.append((y, x + 1))

    return connected


def clear_white_background(
    input_path: str | Path,
    output_path: str | Path,
    white_threshold: int = 245,
) -> Path:
    input_path = Path(input_path)
    output_path = Path(output_path)

    image = iio.imread(input_path)
    if image.ndim == 2:
        rgb = np.stack([image, image, image], axis=-1)
        alpha = np.full(image.shape, 255, dtype=np.uint8)
    else:
        rgb = image[..., :3]
        if image.shape[-1] >= 4:
            alpha = image[..., 3].copy()
        else:
            alpha = np.full(rgb.shape[:2], 255, dtype=np.uint8)

    white_mask = np.all(rgb >= white_threshold, axis=-1)
    background_mask = _border_connected_mask(white_mask)
    alpha[background_mask] = 0

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output = np.dstack([rgb, alpha]).astype(np.uint8)
    iio.imwrite(output_path, output)
    return output_path

clear_white_background("images/shocked.png","images/shocked.png")