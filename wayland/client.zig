const std = @import("std");
const print = std.debug.print;
const linux = std.os.linux;
const Init = std.process.Init;
const Environ = std.process.Environ;

const display_id: u32 = 1;

fn connect_server(env_map: *Environ.Map) !i32 {
    // zig implementation of linux.socket return usize which matches the address space of the system,
    // so 64-bit in my case, so need to cast to 32-bit int which is expected by the connect syscall.
    const fd: i32 = @intCast(linux.socket(linux.AF.UNIX, linux.SOCK.STREAM | linux.SOCK.CLOEXEC, 0));

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

    return fd;
}

const Header = packed struct { object_id: u32, opcode: u16, size: u16 };

inline fn toBytes(val: anytype) []const u8 {
    // array literal requires address-of operator (&) to coerce to slice type '[]const u8'
    return &@as([@sizeOf(@TypeOf(val))]u8, @bitCast(val));
}

fn get_registry(fd: i32, id: u32) void {
    const header: Header = .{ .object_id = display_id, .opcode = 1, .size = 12 };
    const headerBytes = toBytes(header);

    const GetRegistryPayload = packed struct { registry: u32 };
    const payload: GetRegistryPayload = .{ .registry = id };
    const payloadBytes = toBytes(payload);

    var msg: [headerBytes.len + @sizeOf(GetRegistryPayload)]u8 = undefined;
    @memcpy(msg[0..headerBytes.len], headerBytes);
    @memcpy(msg[headerBytes.len..msg.len], payloadBytes);

    // const msg = [_]u8{ 1, 0, 0, 0, 1, 0, 12, 0, 2, 0, 0, 0 };

    _ = linux.write(fd, &msg, msg.len);

    // print("Wrote {} bytes\n", .{n});
}

pub fn main(init: Init) !void {
    const fd = try connect_server(init.environ_map);
    // print("fd: {}\n", .{fd});

    const registry_id: u32 = 2;
    get_registry(fd, registry_id);

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
            const interface_name_len = std.mem.readInt(u32, interface[0..4], .little);
            const interface_name = interface[4..(4 + interface_name_len)];
            // interface version
            const version = std.mem.readInt(u32, payload[(payload_size - 4)..payload_size][0..4], .little);
            // print("Read {} bytes – name: {}, version: {}, i_name: {s}, i_name_len: {}\n", .{ n3, name, version, interface_name, interface_name_len });
            print("{}\tv{}\t{s},\n", .{ name, version, interface_name });
            // for (interface) |value| {
            //     print("{x}|", .{value});
            // }
            // print("\n", .{});
        }
    }
}
