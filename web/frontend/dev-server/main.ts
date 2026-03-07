import { serveDir } from "@std/http/file-server";

const ROOT_DIR = "../public";
const PORT = 8080;

const watchers = new Set<WritableStreamDefaultWriter>();

function liveReloadHandler(req: Request): Response {
  const stream = new TransformStream();
  const writer = stream.writable.getWriter();
  watchers.add(writer);

  req.signal.addEventListener("abort", () => {
    writer.close();
    watchers.delete(writer);
  });

  return new Response(stream.readable, {
    headers: {
      "Content-Type": "text/event-stream",
      "Cache-Control": "no-cache",
      "Connection": "keep-alive",
    },
  });
}

function injectLiveReload(html: string): string {
  const script = `
<script>
  const es = new EventSource("/_livereload");
  es.onmessage = () => location.reload();
</script>
</body>`;
  return html.replace("</body>", script);
}

async function handler(req: Request): Promise<Response> {
  const url = new URL(req.url);

  if (url.pathname === "/_livereload") {
    return liveReloadHandler(req);
  }

  const res = await serveDir(req, { fsRoot: ROOT_DIR });

  const ct = res.headers.get("Content-Type") ?? "";

  if (ct.includes("text/html")) {
    const original = await res.text();

    return new Response(injectLiveReload(original), res);
  }

  return res;
}

const onListen = () => {
  const url = `http://localhost:${PORT}`;
  console.log(`Serving ${url}`);
  // spawn a subprocess to open default browser
  // depending on OS: choose appropriate command
  const cmd = ["xdg-open", url];

  const command = new Deno.Command(cmd[0], { args: cmd.slice(1) });
  command.spawn();
};

Deno.serve({ port: PORT, onListen, handler });

const encoder = new TextEncoder();

for await (const _ of Deno.watchFs(ROOT_DIR)) {
  for (const w of watchers) {
    // w.write(`data: reload\n\n`);
    w.write(encoder.encode("data: reload\n\n"));
  }
}
