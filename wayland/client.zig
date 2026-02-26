const std = @import("std");
const print = std.debug.print;
const linux = std.os.linux;
const Init = std.process.Init;
const Environ = std.process.Environ;

var fd: i32 = undefined;

var new_id: u32 = 1;
inline fn newId() u32 {
    new_id += 1;
    return new_id;
}

const display_id: u32 = 1;
var registry_id: u32 = 0;
var wl_compositor_id: u32 = 0;
const wl_compositor_iname = "wl_compositor";
var wl_shm_id: u32 = 0;
const wl_shm_iname = "wl_shm";
var wl_surface_id: u32 = 0;

fn connectServer(env_map: *Environ.Map) !void {
    // zig implementation of linux.socket return usize which matches the address space of the system,
    // so 64-bit in my case, so need to cast to 32-bit int which is expected by the connect syscall.
    fd = @intCast(linux.socket(linux.AF.UNIX, linux.SOCK.STREAM | linux.SOCK.CLOEXEC, 0));

    var path: [108]u8 = @splat(0);
    const runtime = env_map.get("XDG_RUNTIME_DIR").?;
    const display = env_map.get("WAYLAND_DISPLAY").?;
    const path_len = runtime.len + 1 + display.len;
    @memcpy(path[0..runtime.len], runtime);
    @memcpy(path[runtime.len..(runtime.len + 1)], "/");
    @memcpy(path[(runtime.len + 1)..(path_len)], display);
    const addr: linux.sockaddr.un = .{ .family = linux.AF.UNIX, .path = path };
    // print("{s}\n", .{path});

    // For some reason, can't cast directly to i32, but has to cast first to integer of usize
    // and then cast that to the integer size required by the connect function
    const connectError64: i64 = @bitCast(linux.connect(fd, &addr, @sizeOf(linux.sockaddr.un)));
    const connectError32: i32 = @intCast(connectError64);
    if (connectError32 < 0) {
        print("Connect error: {}\n", .{connectError32});
        std.os.linux.exit(1);
    }
}

const Header = packed struct { object_id: u32, opcode: u16, size: u16 };

inline fn toBytes(val: anytype) []const u8 {
    // array literal requires address-of operator (&) to coerce to slice type '[]const u8'
    return &@as([@sizeOf(@TypeOf(val))]u8, @bitCast(val));
}

fn getRegistry() void {
    const header: Header = .{ .object_id = display_id, .opcode = 1, .size = 12 };
    const headerBytes = toBytes(header);

    const GetRegistryPayload = packed struct { registry: u32 };
    registry_id = newId();
    const payload: GetRegistryPayload = .{ .registry = registry_id };
    const payloadBytes = toBytes(payload);

    var msg: [headerBytes.len + @sizeOf(GetRegistryPayload)]u8 = undefined;
    @memcpy(msg[0..headerBytes.len], headerBytes);
    @memcpy(msg[headerBytes.len..msg.len], payloadBytes);

    // const msg = [_]u8{ 1, 0, 0, 0, 1, 0, 12, 0, 2, 0, 0, 0 };

    _ = linux.write(fd, &msg, msg.len);
    // _ = linux.write(fd, @ptrCast(headerBytes), @sizeOf(Header));
    // _ = linux.write(fd, @ptrCast(payloadBytes), @sizeOf(GetRegistryPayload));

    // print("Wrote {} bytes\n", .{n});
}

fn printBytes(arr: []const u8) void {
    for (arr) |value| {
        print("{x} ", .{value});
    }
    print("\n", .{});
}

inline fn align4(n: u32) u32 {
    return (n + 0b11) & ~@as(u32, 0b11);
}

/// The description of bind request in wayland.xml makes it look like it only takes two arguments,
/// name and new_id, but it actually takes two more: the interface_name and the interface_version.
/// The need for these extra arguments can apparently be deduced from the fact that the new_id arg
/// tag does not contain the interface attribute. In other words, the interface is not specified.
inline fn registryBind(name: u32, interface_name: []const u8, interface_version: u32, id: u32) void {
    const strlen: u32 = interface_name.len + 1;
    var str: [4 + align4(strlen)]u8 = @splat(0);
    @memcpy(str[0..4], toBytes(strlen));
    @memcpy(str[4..(4 + interface_name.len)], interface_name);
    const bind_payload_size = 4 + str.len + 4 + 4;
    var bind_payload_bytes: [bind_payload_size]u8 = undefined;
    @memcpy(bind_payload_bytes[0..4], toBytes(name));
    @memcpy(bind_payload_bytes[4..(4 + str.len)], &str);
    @memcpy(bind_payload_bytes[(4 + str.len)..(4 + str.len + 4)], toBytes(interface_version));
    @memcpy(bind_payload_bytes[(4 + str.len + 4)..bind_payload_size], toBytes(id));

    const bind_header: Header = .{ .object_id = registry_id, .opcode = 0, .size = @sizeOf(Header) + bind_payload_size };
    const bind_header_bytes = toBytes(bind_header);

    var msg: [@sizeOf(Header) + bind_payload_size]u8 = undefined;
    @memcpy(msg[0..bind_header_bytes.len], bind_header_bytes);
    @memcpy(msg[bind_header_bytes.len..msg.len], &bind_payload_bytes);

    const n = linux.write(fd, &msg, msg.len);
    if (n != msg.len) {
        print("registry_bind error\n", .{});
        std.os.linux.exit(1);
    }
    // print("Bind {s}: wrote {} bytes\n", .{ interface_name, n });
    // printBytes(&msg);
}

inline fn wlCompositorCreateSurface() void {
    const header: Header = .{ .object_id = wl_compositor_id, .opcode = 0, .size = 12 };
    const header_bytes = toBytes(header);

    wl_surface_id = newId();
    const payload_bytes = toBytes(@as(u32, wl_surface_id));

    var msg: [@sizeOf(Header) + @sizeOf(@TypeOf(wl_surface_id))]u8 = undefined;
    @memcpy(msg[0..header_bytes.len], header_bytes);
    @memcpy(msg[header_bytes.len..msg.len], payload_bytes);

    _ = linux.write(fd, &msg, msg.len);
    // printBytes(&msg);
}

pub fn main(init: Init) !void {
    try connectServer(init.environ_map);

    getRegistry();

    var i: u32 = 0;
    while (true) : (i += 1) {
        var header: Header = undefined;
        _ = linux.read(fd, @ptrCast(&header), @sizeOf(Header));
        const payload_size = header.size - @sizeOf(Header);

        // print("Read {} bytes – Payload size: {}\n", .{ n, payload_size });

        var payload: [4096]u8 = undefined;
        _ = linux.read(fd, &payload, payload_size);

        if (header.object_id == display_id) {
            const error_object_id = std.mem.readInt(u32, payload[0..4], .little);
            const error_code = std.mem.readInt(u32, payload[4..8], .little);
            const error_msg_len = std.mem.readInt(u32, payload[8..12], .little);
            const error_msg = payload[12 .. 12 + error_msg_len];
            print("Error – object_id: {}, error_code: {}, error_msg: {s}\n", .{ error_object_id, error_code, error_msg });
        } else if (header.object_id == registry_id) {
            // numeric name of the global object
            const name = std.mem.readInt(u32, payload[0..4], .little);
            // interface implemented by the object
            const interface = payload[4..(payload_size - 4)];
            // -1 to ignore the null delimiter as zig strings don't have those
            const interface_name_len = std.mem.readInt(u32, interface[0..4], .little) - 1;
            // const interface_name_len = std.mem.readInt(u32, interface[0..4], .little);
            const interface_name = interface[4..(4 + interface_name_len)];
            // interface version
            const version = std.mem.readInt(u32, payload[(payload_size - 4)..payload_size][0..4], .little);
            print("{}\tv{}\t{s},\n", .{ name, version, interface_name });

            // printBytes(interface_name);
            // printBytes("wl_compositor");

            if (std.mem.eql(u8, interface_name, wl_compositor_iname)) {
                wl_compositor_id = newId();
                registryBind(name, wl_compositor_iname, version, wl_compositor_id);
                wlCompositorCreateSurface();
            } else if (std.mem.eql(u8, interface_name, wl_shm_iname)) {
                wl_shm_id = newId();
                registryBind(name, wl_shm_iname, version, wl_shm_id);
            }
        }
    }
}
