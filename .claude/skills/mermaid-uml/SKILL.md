---
name: mermaid-uml
description: Create or edit UML/Mermaid diagrams as .mmd files (class, sequence, state, ER, flowchart), compatible with Siren Studio's `%% siren-layout` comment. Use when the user asks for a diagram, UML, or a .mmd file.
---

# Mermaid diagrams as .mmd files

## Rules
- Save diagrams as `<name>.mmd`, containing raw Mermaid only. No markdown fences inside the file.
- Pick the diagram type that fits: `classDiagram`, `sequenceDiagram`, `stateDiagram-v2`, `erDiagram`, `flowchart TD|LR`.
- Keep node/class ids simple (letters, digits, underscore), with no spaces. Put readable text in labels, e.g. `A["Order service"]`.
- Never use a node id that is a Mermaid keyword: `end`, `graph`, `subgraph`, `style`, `class`, `classDef`, `click`, `default`.
- Class members use Mermaid's native form: `+name: Type`, `+method(arg: Type) ReturnType`; visibility `+ - # ~`.
- Validate if possible: `npx -y @mermaid-js/mermaid-cli -i file.mmd -o out.svg`. Fix any syntax error before reporting done.
- Edit existing files minimally and preserve unrelated content.

## Siren layout line
Siren Studio stores manual node positions in the file as ONE comment line, which Mermaid ignores:

    %% siren-layout {"draft":[204,91],"submitted":[204,202]}

- Keys are node ids exactly as written in the diagram; values are `[x, y]` node CENTRES in diagram units (SVG pixels).
- Must be a single line, valid JSON, integers preferred, usually at the end of the file.
- If a file already has this line, KEEP it. When you rename a node, rename its key too. When you delete a node, remove its key. New nodes without a key are auto-placed by Siren.
- You cannot see the rendered diagram, so do not invent positions. Omit the line for new diagrams; Siren will auto-layout and write it when the user drags nodes.
- Only if the user explicitly asks for a specific arrangement, write a coarse grid (e.g. ~200 units between columns, ~110 between rows).
