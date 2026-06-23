#import "assets/lib/document.typ": conf
#import "assets/lib/snippets.typ": *

#show: conf.with(
  title: "ディジタル映像処理及び演習",
  sub-title: "第10回課題",
  student-id: "K24132",
  author: "町田 渉",
)

入力画像をモノクロ処理した画像 (@fig-原画像) を原画像とする.  これにガウス型のハイパス, ローパス, バンドパスフィルターを適用し, それぞれ得られたスペクトル画像と逆変換画像を @fig-結果 に示す.

#figure(
  image("../out/mono.jpg", width: 20em),
  caption: "原画像",
)<fig-原画像>

#show table.cell: set text(font: SANS)
#show table.cell: set align(center + horizon)
#show table.cell.where(y: 0): set text(weight: "bold")
#show figure: set block(breakable: true)

#let way(it) = table.cell(
  align: center + horizon,
  rotate(-90deg, reflow: true, it),
)

#figure(
  pad(
    x: 0.7em,
    table(
      columns: 3,
      table.header([手法], [スペクトル画像], [逆変換画像]),

      ..(
        way[ローパスフィルター],
        image("../out/lpf_spectrum.jpg"),
        image("../out/lpf_result.jpg"),

        way[ハイパスフィルター],
        image("../out/hpf_spectrum.jpg"),
        image("../out/hpf_result.jpg"),

        way[バンドパスフィルター ($sigma = 0.05$)],
        image("../out/bpf_005_spectrum.jpg"),
        image("../out/bpf_005_result.jpg"),

        way[バンドパスフィルター ($sigma = 0.03$)],
        image("../out/bpf_003_spectrum.jpg"),
        image("../out/bpf_003_result.jpg"),

        way[バンドパスフィルター ($sigma = 0.01$)],
        image("../out/bpf_001_spectrum.jpg"),
        image("../out/bpf_001_result.jpg"),
      ),
    ),
  ),
  caption: "ガウス型フィルターの適用結果",
)<fig-結果>

