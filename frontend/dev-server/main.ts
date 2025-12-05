import { serveDir } from "@std/http/file-server";

Deno.serve((req) => {
  const pathname = new URL(req.url).pathname;
  console.log(pathname);
  return serveDir(req, { fsRoot: "../public" });
});
