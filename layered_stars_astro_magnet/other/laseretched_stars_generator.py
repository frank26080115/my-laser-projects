#!/usr/bin/env python

import sys, math, random
from PIL import Image, ImageDraw

PAGE_WIDTH_INCH = 3
PAGE_HEIGHT_INCH = 3
CIRCLE_DIAMETER_INCH = 2.8

DPI = 100 # could be 100, 250, 500, 1000

STAR_CNT = 25
STAR_DIST = 0.3

STAR_SIZE_MIN = 2
STAR_SIZE_MAX = 4

def check_avg_dist(list):
    dists = []
    for i in list:
        mindist = 2 ** 31
        for j in list:
            dx = i[0] - j[0]
            dy = i[1] - j[1]
            mag = math.sqrt((dx ** 2) + (dy ** 2))
            if mag != 0 and mag < mindist:
                mindist = mag
        dists.append(mindist)
    sum = 0
    for i in dists:
        sum += i
    return sum / len(dists)

def check_nearest_dist(n, list):
    mindist = 2 ** 31
    for i in list:
        dx = i[0] - n[0]
        dy = i[1] - n[1]
        mag = math.sqrt((dx ** 2) + (dy ** 2))
        if mag < mindist:
            mindist = mag
    return mindist

def check_center_dist(n, center):
    dx = n[0] - center[0]
    dy = n[1] - center[1]
    mag = math.sqrt((dx ** 2) + (dy ** 2))
    return mag

def main():
    dim = (int(round(PAGE_WIDTH_INCH * DPI)), int(round(PAGE_HEIGHT_INCH * DPI)))
    cent = (dim[0] / 2, dim[1] / 2)
    im = Image.new("RGBA", dim, (0xFF, 0xFF, 0xFF, 0x00))
    draw = ImageDraw.Draw(im)

    star_list = []

    while len(star_list) < STAR_CNT or check_avg_dist(star_list) > STAR_DIST * DPI:
        while True:
            n = (random.randint(0, dim[0]), random.randint(0, dim[1]))
            if check_nearest_dist(n, star_list) > (STAR_DIST * DPI * 0.5) and check_center_dist(n, cent) < (CIRCLE_DIAMETER_INCH * DPI / 2):
                star_list.append(n)
                break

    for i in star_list:
        r = random.randint(STAR_SIZE_MIN, STAR_SIZE_MAX)
        draw.ellipse([(i[0], i[1]), (i[0] + r, i[1] + r)], fill=(0, 0, 0, 0xFF), outline=None)

    im.save("result.png", dpi=(DPI, DPI))

if __name__ == "__main__":
    main()