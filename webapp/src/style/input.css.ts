import { style } from "@vanilla-extract/css"
import op from "open-props"

export const input = style({
  width: "100%",
  appearance: "none",
  borderRadius: op.radius2,
  borderWidth: "1px",
  borderStyle: "solid",
  borderColor: op.gray3,
  backgroundColor: "transparent",
  padding: op.size2,
  outline: "none",
  selectors: {
    "&:forcus": {
      borderColor: op.blue5,
    },
    "&:invalid": {
      borderColor: op.red3,
    },
  },
})

export const inputContainerY = style({
  display: "flex",
  flexDirection: "column",
  rowGap: op.size1,
})

export const inputContainerX = style({
  display: "flex",
  justifyContent: "space-between",
  columnGap: op.size1,
})

export const checkbox = style({
  width: "1rem",
  height: "1rem",
  marginRight: op.size2,
  accentColor: op.blue5,
  selectors: {
    "&:focus": {
      outlineColor: op.blue5,
    },
  },
})

export const checkboxLabel = style({
  display: "block",
})
