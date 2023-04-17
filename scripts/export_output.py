import json
import os
import math
import pandas as pd
import sys
import git
from pathlib import Path
from inspect import cleandoc

from plotly.subplots import make_subplots
import plotly.express as px
import plotly.graph_objects as go
import plotly
from pretty_html_table import build_table

# plot colors
pal = px.colors.qualitative.Plotly
color_sequence = ["#BBB", "#777", "#111", pal[9], pal[4], pal[6], pal[1], pal[0], "#58a2c4", pal[5], pal[2], pal[7], pal[8], pal[3]]

# plot labels
plot_labels = dict(
    cpu_time='ns per key',
    data_elem_count='dataset size',
    table_bits_per_key='total bits per key',
    point_lookup_percent='percentage of point queries')

file = sys.argv[1]
with open(file) as data_file:
    data = json.load(data_file)

    # convert json results to dataframe
    df = pd.json_normalize(data, 'benchmarks')

    # augment additional computed columns
    # augment plotting datasets
    def magnitude(x):
        l = math.log(x, 10)
        rem = round(x/pow(10, l), 2)
        exp = int(round(l, 0))
        #return f'${rem} \cdot 10^{{{exp}}}$'
        return f'{rem}e-{exp}'
    df["method"] = df["label"].apply(lambda x : x.split(":")[0])
    df["dataset"] = df["label"].apply(lambda x : x.split(":")[1])
    df["elem_magnitude"] = df.apply(lambda x : magnitude(x["data_elem_count"]), axis=1)

    # subset specific filtering & augmentation
    df["probe_distribution"] = df["label"].apply(lambda x : x.split(":")[2] if len(x.split(":")) > 2 else "-")
    df["probe_size"] = df["name"].apply(lambda x : int(x.split(",")[1].split(">")[0]))

    df["cpu_time_per_key"] = df.apply(lambda x : x["cpu_time"] / x["data_elem_count"], axis=1)
    df["throughput"] = df.apply(lambda x : 10**9/x["cpu_time_per_key"], axis=1)
    df = df[df["data_elem_count"] > 9 * 10**7]

    df["_sort_name"] = df["label"].apply(lambda x : x.split(":")[0] if len(x.split(":")) > 0 else "-")
    # df["probe_distribution"] = df["label"].apply(lambda x : x.split(":")[2] if len(x.split(":")) > 2 else "-")
    # df = df.sort_values(["_sort_name", "point_lookup_percent"], ascending=True)

    # ensure export output folder exists
    results_path = "../output/docs"
    Path(results_path).mkdir(parents=True, exist_ok=True)

    def convert_to_html(fig):
        return fig.to_html(full_html=False, include_plotlyjs=False)

    def plot_lookup_times(probe_size):
        data = df[df["probe_size"] == probe_size]
        fig = px.line(
            data,
            x="data_elem_count",
            y="cpu_time",
            color="method",
            facet_row="probe_distribution",
            facet_col="dataset",
            category_orders={"dataset": ["seq", "gap_10", "uniform", "normal", "wiki", "osm", "fb"]},
            markers=True,
            log_x=True,
            labels=plot_labels,
            color_discrete_sequence=color_sequence,
            height=1000,
            title=f"Probe (size: {probe_size}) - ns per key"
            )

        # hide prefetched results by default
        fig.for_each_trace(lambda trace: trace.update(visible="legendonly")
                   if trace.name.startswith("Prefetched") else ())

        return fig

    def plot_mixed():
        data = df[df["data_elem_count"] == 10**8]
        fig = px.line(
             data,
             x="point_lookup_percent",
             y="cpu_time",
             color="method",
             facet_row="probe_distribution",
             facet_col="dataset",
             category_orders={"dataset": ["seq", "gap_10", "uniform", "normal", "wiki", "osm", "fb"]},
             markers=True,
             log_x=False,
             labels=plot_labels,
             color_discrete_sequence=color_sequence,
             height=1000,
             title=f"Mixed workload - ns per key"
        )

        # hide prefetched results by default
        fig.for_each_trace(lambda trace: trace.update(visible="legendonly")
                   if trace.name.startswith("Prefetched") else ())

        return fig


    def plot_construction_times():
        fig = px.bar(
            df,
            x="elem_magnitude",
            y="throughput",
            color="method",
            barmode="group",
            facet_col="dataset",
            category_orders={"dataset": ["seq", "gap_10", "uniform", "normal", "wiki", "osm", "fb"]},
            labels=plot_labels,
            color_discrete_sequence=color_sequence,
            height=500,
            title=f"Construction time - keys per second"
            )

        # hide prefetched results by default
        fig.for_each_trace(lambda trace: trace.update(visible="legendonly")
                   if trace.name.startswith("Prefetched") else ())

        return fig

    def plot_space_usage():
        fig = px.line(
            df,
            x="data_elem_count",
            y="table_bits_per_key",
            color="method",
            facet_col="dataset",
            category_orders={"dataset": ["seq", "gap_10", "uniform", "normal", "wiki", "osm", "fb"]},
            markers=True,
            log_x=True,
            labels=plot_labels,
            color_discrete_sequence=color_sequence,
            height=500,
            title=f"Total space usage - bits per key"
            )

        # hide prefetched results by default
        fig.for_each_trace(lambda trace: trace.update(visible="legendonly")
                   if trace.name.startswith("Prefetched") else ())

        return fig

    def plot_pareto_lookup_vs_space(probe_size):
        filtered = df[(df["probe_size"] == probe_size) & (df["data_elem_count"] > 9 * 10**7)] 
        fig = px.scatter(
            filtered,
            x="cpu_time",
            y="table_bits_per_key",
            color="method",
            facet_row="probe_distribution",
            facet_col="dataset",
            category_orders={"dataset": ["seq", "gap_10", "uniform", "normal", "wiki", "osm", "fb"]},
            labels=plot_labels,
            color_discrete_sequence=color_sequence,
            height=1000,
            title=f"Pareto - lookup ({probe_size} elems in ns) vs space (total in bits/key)"
            )

        # hide prefetched results by default
        fig.for_each_trace(lambda trace: trace.update(visible="legendonly")
                   if trace.name.startswith("Prefetched") else ())

        return fig

    outfile_name = os.path.splitext(os.path.basename(file))[0] + ".html"
    with open(f'{results_path}/{outfile_name}', 'w') as readme:
        readme.write(cleandoc(f"""
        <!doctype html>
        <html>
          <head>
              <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
          </head>

          <body>
            {convert_to_html(plot_lookup_times(0))}

            {convert_to_html(plot_space_usage())}

            {convert_to_html(plot_pareto_lookup_vs_space(0))}

            {convert_to_html(plot_construction_times())}

          </body>
        </html>
        """))
