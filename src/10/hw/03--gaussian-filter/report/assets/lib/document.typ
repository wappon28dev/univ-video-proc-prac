#let SANS = "LINE Seed JP_TTF"
#let SERIF = "GenYoMin JP"
#let MONO = "UDEV Gothic 35NF"

#let conf(
  doc,
  idx: 1,
  title: "",
  sub-title: "",
  student-id: "",
  author: "",
) = {
  let name = student-id + " " + author

  set document(title: title, author: name)
  set page(
    paper: "a4",
    margin: 2cm,
    header: context {
      set text(font: SANS, fill: gray, size: 0.9em)
      set align(center)

      if (here().page() != 1) {
        title + " - " + sub-title
      }
    },
    footer: {
      set align(center)
      set text(font: SANS, fill: gray, size: 0.7em)

      context counter(page).display("1 / 1", both: true)
    },
  )

  set text(
    font: SERIF,
    region: "jp",
    lang: "ja",
    weight: "medium",
    size: 10.5pt,
  )

  set par(
    spacing: 1em,
    leading: 1.09em,
    first-line-indent: 1em,
    justify: true,
  )

  show raw: set text(font: MONO)
  show figure.caption: set text(size: 0.8em, font: SANS, weight: "regular")

  set heading(
    numbering: (..n) => {
      let level = n.pos().len()
      if level > 3 {
        return ""
      }

      return numbering("設問1.1.1.", ..n)
    },
    // numbering: "1.1.1.",
    supplement: it => (
      [章],
      [節],
    ).at(it.depth - 1, default: [節]),
  )

  show heading.where(level: 1): it => context {
    set text(size: 0.8em)

    it
  }

  show ref: it => {
    set text(font: SANS, weight: "regular")

    let el = it.element

    if el == none {
      return it
    }

    if el.func() == heading {
      return link(
        el.location(),
        [
          第
          #numbering("1.1", ..counter(heading).at(el.location()))
          #el.supplement
        ],
      )
    }

    it
  }

  show figure.caption: set text(size: 1em, fill: gray.darken(60%))

  show heading: it => {
    set block(below: 0em)
    set text(weight: "bold", font: SANS)
    // pad(
    //   left: -0.25em + if it.depth == 1 { -1em } else { 0em },
    //   it,
    // )

    it
    par(text(size: 0em, ""))
  }

  show raw: it => {
    text(font: MONO, it)
  }
  show strong: it => {
    set text(weight: 300, font: SANS)
    it
  }
  show math.equation: it => {
    show regex("[\p{scx:Han}\p{scx:Hira}\p{scx:Kana}（）]"): set text(font: SERIF)
    it
  }

  show ". ": "．"

  {
    set align(center)
    set text(font: SANS)

    stack(
      spacing: 1.3em,
      {
        set text(weight: "bold", size: 18pt)
        title + " - " + sub-title
      },
    )
  }
  [
    #v(0.3em)
    #set align(right)
    #set text(font: SANS)
    #name
  ]

  doc
}


#let q(body) = {
  counter(heading).step()

  block(
    stroke: 1pt,
    inset: 1em,
    radius: 0.5em,
    width: 100%,
    [
      #context [
        #set text(font: SANS, size: 1em, weight: "bold")
        #text(font: ("New Computer Modern", SANS), size: 1.1em, weight: "bold", counter(heading).display())
      ]\
      #set enum(numbering: "a. ")
      #body
    ],
  )
}
