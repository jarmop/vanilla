Deno.serve(
  {
    port: 443,
    cert: Deno.readTextFileSync("../../cert.pem"),
    key: Deno.readTextFileSync("../../key.pem"),
  },
  (_req) => {
    return new Response("Hello, World!");
  },
);
