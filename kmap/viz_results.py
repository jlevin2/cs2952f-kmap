import pandas as pd
import matplotlib.pyplot as plt
import os, sys

FILES = [
    "tiny",
    "small",
    "medium"
]

FILESIZE = [
    "700B",
    "100KB",
    "10MB"
]

fig, ax = plt.subplots(ncols=len(FILES),figsize=(10,5))

for i,f in enumerate(FILES):
    default = pd.read_csv("results/24Buffer/{}/default.csv".format(f))
    kmap = pd.read_csv("results/24Buffer/{}/kmap.csv".format(f))

    merged = pd.DataFrame()
    merged['kmap'] = kmap['Times']
    merged['sys'] = default['Times']

    bplot = ax[i].boxplot(x=merged.values, showfliers=False, notch=False, widths=0.5, patch_artist=True)
    bplot['boxes'][0].set_facecolor('grey')
    bplot['boxes'][1].set_facecolor('purple')
    for _, box in enumerate(bplot['medians']):
        box.set_color('black')
        box.set_linewidth(2.0)

    for _, box in enumerate(bplot['whiskers']):
        box.set_color('black')
        box.set_linewidth(2.0)

    for _, box in enumerate(bplot['caps']):
        box.set_color('black')
        box.set_linewidth(2.0)

    ax[i].set_xlabel('')
    ax[i].set_ylabel('Time (s)')

    ax[i].set_title('File Size: ' + FILESIZE[i])
    ax[i].legend([bplot["boxes"][0], bplot["boxes"][1]],
          ['Kmap', 'Sys'], loc='upper right', fontsize=12, markerscale=5.0)


plt.savefig('results.png', dpi=300, bbox_inches = 'tight',pad_inches = 0)