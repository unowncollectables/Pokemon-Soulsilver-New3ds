"""Edge-collapse (quadric error) triangle reduction for the banner's rigidly skinned parts. Standard library only.

Why: the Platinum port's banner (the one that runs on the HOME Menu) has 1000 triangles; the title Lugia has 2048, and the
README of the Platinum port records that quadrupling Giratina's triangles crashed the HOME Menu. Cutting Lugia to
Platinum's budget removes that difference. Triangles are (material, [vertex, vertex, vertex]) with a vertex
(joint, local_pos, uv, colour, local_normal), as lugia_parts.build() returns them.

Rules, chosen so the result still animates and textures exactly like the original:
  * position-welded vertices of one joint are collapsed along an edge, a -> b (b keeps its position, so nothing moves
    except the vertices that go away, and the joint binding of every remaining vertex is unchanged);
  * only a vertex with one uv/normal ("corner") and no open edge is removed, so texture seams, hard edges and the
    model's outline are never touched;
  * a collapse that would flip a face, or pinch the surface (link condition), is skipped.
"""
import heapq
import math


def _key(v):
    return (v[0], round(v[1][0], 4), round(v[1][1], 4), round(v[1][2], 4))


def _ckey(v):
    return (tuple(round(x, 4) for x in v[2]), tuple(round(x, 2) for x in v[4]))


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _plane_quadric(p0, p1, p2):
    """(10 coefficients of the area-weighted squared distance to the triangle's plane, normal, area)."""
    c = _cross(_sub(p1, p0), _sub(p2, p0))
    area2 = math.sqrt(_dot(c, c))
    if area2 < 1e-12:
        return (0.0,) * 10, None, 0.0
    n = (c[0] / area2, c[1] / area2, c[2] / area2)
    d = -_dot(n, p0)
    w = area2 / 2
    a, b, cc = n
    return (w * a * a, w * a * b, w * a * cc, w * a * d, w * b * b, w * b * cc, w * b * d, w * cc * cc, w * cc * d, w * d * d), n, w


def _qadd(q, r):
    return tuple(x + y for x, y in zip(q, r))


def _qerr(q, p):
    x, y, z = p
    return (q[0] * x * x + 2 * q[1] * x * y + 2 * q[2] * x * z + 2 * q[3] * x + q[4] * y * y + 2 * q[5] * y * z + 2 * q[6] * y
            + q[7] * z * z + 2 * q[8] * z + q[9])


def decimate(tris, target):
    """Reduce `tris` to at most `target` triangles if the rules above allow it; returns (new triangles, count reached)."""
    if len(tris) <= target:
        return tris, len(tris)
    pid, pos, inst = {}, [], []
    faces = []                                   # [material, [pid]*3, [vertex tuple]*3] or None when removed
    for mat, t in tris:
        ids = []
        for v in t:
            k = _key(v)
            if k not in pid:
                pid[k] = len(pos)
                pos.append(tuple(v[1]))
                inst.append(v[0])
            ids.append(pid[k])
        faces.append([mat, ids, list(t)])
    n = len(pos)
    vf = [set() for _ in range(n)]
    corners = [set() for _ in range(n)]
    edge_faces = {}
    Q = [(0.0,) * 10 for _ in range(n)]
    for fi, (mat, ids, vs) in enumerate(faces):
        q, _, _ = _plane_quadric(*(pos[i] for i in ids))
        for i, v in zip(ids, vs):
            vf[i].add(fi)
            corners[i].add(_ckey(v))
            Q[i] = _qadd(Q[i], q)
        for k in range(3):
            e = (min(ids[k], ids[(k + 1) % 3]), max(ids[k], ids[(k + 1) % 3]))
            edge_faces.setdefault(e, set()).add(fi)
    locked = [False] * n
    for (a, b), fs in edge_faces.items():
        if len(fs) != 2:
            locked[a] = locked[b] = True          # open edge: the model's outline
    for i in range(n):
        if len(corners[i]) != 1:
            locked[i] = True                      # texture seam or hard edge
    neigh = [set() for _ in range(n)]
    for (a, b) in edge_faces:
        neigh[a].add(b)
        neigh[b].add(a)
    ver = [0] * n
    heap = []

    def cost(a, b):
        return _qerr(_qadd(Q[a], Q[b]), pos[b])

    def push(a, b):
        if not locked[a] and inst[a] == inst[b]:
            heapq.heappush(heap, (cost(a, b), a, b, ver[a], ver[b]))

    for a in range(n):
        for b in neigh[a]:
            push(a, b)
    alive = len(tris)

    def corner_of(fi, vid):
        mat, ids, vs = faces[fi]
        return vs[ids.index(vid)]

    while heap and alive > target:
        c, a, b, va, vb = heapq.heappop(heap)
        if ver[a] != va or ver[b] != vb or b not in neigh[a]:
            continue
        shared = vf[a] & vf[b]
        if len(shared) != 2:
            continue
        opp = set()
        for fi in shared:
            opp |= set(faces[fi][1]) - {a, b}
        if neigh[a] & neigh[b] != opp:
            continue                              # would pinch the surface
        cb = [corner_of(fi, b) for fi in shared]
        if _ckey(cb[0]) != _ckey(cb[1]) or faces[next(iter(shared))][0] != faces[tuple(shared)[1]][0]:
            continue
        moved = vf[a] - shared
        ok = True
        for fi in moved:
            ids = faces[fi][1]
            old = [pos[i] for i in ids]
            new = [pos[b] if i == a else pos[i] for i in ids]
            co = _cross(_sub(old[1], old[0]), _sub(old[2], old[0]))
            cn = _cross(_sub(new[1], new[0]), _sub(new[2], new[0]))
            lo, ln = math.sqrt(_dot(co, co)), math.sqrt(_dot(cn, cn))
            if lo < 1e-12 or ln < 1e-12 or _dot(co, cn) / (lo * ln) < 0.2:
                ok = False
                break
        if not ok:
            continue
        # collapse a -> b
        bc = cb[0]
        for fi in shared:
            faces[fi] = None
            for i in (a, b):
                vf[i].discard(fi)
            for o in opp:
                vf[o].discard(fi)
        for fi in moved:
            mat, ids, vs = faces[fi]
            k = ids.index(a)
            ids[k] = b
            vs[k] = (vs[k][0], list(pos[b]), bc[2], bc[3], bc[4])
            vf[b].add(fi)
        vf[a] = set()
        for o in list(neigh[a]):
            neigh[o].discard(a)
        for o in neigh[a] - {b}:
            neigh[o].add(b)
            neigh[b].add(o)
        neigh[b].discard(a)
        neigh[a] = set()
        Q[b] = _qadd(Q[b], Q[a])
        alive -= 2
        touched = {b} | neigh[b]
        for t in touched:
            ver[t] += 1
        for t in touched:
            for o in neigh[t]:
                push(t, o)
    out = [(f[0], f[2]) for f in faces if f is not None]
    return out, len(out)
