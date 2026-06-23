#import "document.typ": *

#let s(it) = text(font: SERIF, it)
#let hr = line(length: 100%, stroke: gray)
#let kbd(
  it,
) = box(
  fill: gray.lighten(70%),
  inset: -1pt + 4pt,
  radius: 3pt,
  baseline: 0.2em,
  stroke: 0.3pt, //
  text(
    font: MONO,
    size: 0.8em,
    it, //
  ),
)

#let choices(it) = {
  set enum(
    numbering: n => block(
      fill: orange,
      radius: 999pt,
      height: 1em,
      width: 1em,
      inset: 2pt,
      text(
        fill: white,
        font: SANS,
        weight: "bold",
        size: 0.8em,
        numbering("A", n),
      ),
    ),
  )

  it
}

#let ans(body) = block(
  stroke: 1pt,
  inset: 1em,
  width: 100%,
)[
  #text(font: SANS, weight: "bold", size: 0.9em)[答: ]
  #body
]

// ref: https://forum.typst.app/t/character-count-of-body-text/489
#let to-string(content) = {
  if content.has("text") {
    content.text
  } else if content.has("children") {
    content.children.map(to-string).join("")
  } else if content.has("body") {
    to-string(content.body)
  } else if content == [ ] {
    " "
  }
}

#let char-count(it) = [
  #let char-len = to-string(it).clusters().len() - 2

  #it #box(inset: (left: 0.3em), text(fill: gray, font: SANS)[（#char-len 字）])
]

