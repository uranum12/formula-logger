import { style } from "@vanilla-extract/css"
import op from "open-props"

export const button = style({
  borderWidth: "1px",
  borderStyle: "solid",
  borderColor: op.gray6,
  borderRadius: op.radius2,

  paddingTop: op.size2,
  paddingBottom: op.size2,
  paddingLeft: op.size4,
  paddingRight: op.size4,

  outline: "none",
  backgroundColor: "transparent",
  transition: "background-color 0.2s ease",

  selectors: {
    "&:hover": {
      backgroundColor: op.gray1,
    },
    "&:focus": {
      backgroundColor: op.gray1,
    },
    "&:active": {
      backgroundColor: op.gray2,
    },
    "&:disabled": {
      cursor: "not-allowed",
      backgroundColor: op.gray3,
      opacity: 0.5,
    },
  },
})
