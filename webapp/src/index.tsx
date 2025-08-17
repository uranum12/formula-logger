/* @refresh reload */
import { render } from "solid-js/web"
import App from "./App.tsx"
import "./index.css"

const root = document.getElementById("root")

// biome-ignore lint/style/noNonNullAssertion: null root
render(() => <App />, root!)
