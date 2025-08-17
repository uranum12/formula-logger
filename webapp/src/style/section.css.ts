import { style } from "@vanilla-extract/css"
import op from "open-props"

export const main = style({
  paddingLeft: op.size8,
  paddingRight: op.size8,
})

export const section = style({
  marginLeft: "auto",
  marginRight: "auto",
  marginTop: op.size4,
  marginBottom: op.size4,
  maxWidth: op.sizeMd,
  display: "flex",
  flexDirection: "column",
  gap: op.size1,
})
