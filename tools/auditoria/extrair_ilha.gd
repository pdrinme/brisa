extends SceneTree
# Extrator (Etapa 0 do Brisa): roda o gerador determinístico da Ilha Ventania
# numa CÓPIA do projeto e grava o layout em JSON. Não altera o Ventania.
func _init() -> void:
	var out_path: String = OS.get_cmdline_user_args()[0]
	var Ilha = load("res://world/ilha/ilha_ventania.gd")
	var m = Ilha.new()
	m.com_ambiente = false
	var alt := []
	for j in 201:
		for i in 201:
			alt.append(snappedf(m._amostra(i, j), 0.01))
	var props := []
	for pr in m._props:
		var xf: Transform3D = pr[1]
		props.append({"m": (pr[0] as String).get_file().get_basename(), "p": [snappedf(xf.origin.x,0.01), snappedf(xf.origin.y,0.01), snappedf(xf.origin.z,0.01)],
			"ry": snappedf(rad_to_deg(xf.basis.get_euler().y), 0.1), "s": snappedf(xf.basis.get_scale().x, 0.01), "mat": pr[2]})
	var pecas := []
	for p in m._pecas:
		pecas.append({"t": p["tipo"], "c": [p["cell"].x, p["cell"].y, p["cell"].z], "r": p["rot"], "mat": p["material"], "mask": p["mask"]})
	var baus := []
	for b in m._baus:
		baus.append({"p": [b[0].x, b[0].y, b[0].z], "ry": b[1]})
	var loot := []
	for l in m._loot:
		var v: Vector3 = l if l is Vector3 else l[0]
		loot.append([snappedf(v.x,0.01), snappedf(v.y,0.01), snappedf(v.z,0.01)])
	var pois := []
	for p in m.points_of_interest():
		var lim := []
		for q in p["limite"]:
			lim.append([snappedf(q.x,0.1), snappedf(q.y,0.1)])
		pois.append({"id": p["id"], "nome": p["nome"], "cor": p["cor"].to_html(false), "pos": [p["pos"].x, p["pos"].y, p["pos"].z], "raio": p["raio"], "limite": lim})
	var estradas := []
	for e in m._estradas:
		var pts := []
		for q in e["pontos"]:
			pts.append([snappedf(q.x,0.1), snappedf(q.y,0.1)])
		estradas.append({"tipo": e["tipo"], "largura": e["largura"], "pontos": pts})
	var esc := []
	for e in m._escadas:
		esc.append({"p": [e["pos"].x, e["pos"].y, e["pos"].z], "rot": e["rot"], "estilo": e.get("estilo", "")})
	var spawns := []
	for s in m._spawns:
		spawns.append([s.x, s.y, s.z])
	var mob := []
	for f in m.furniture():
		mob.append(str(f))
	# Etapa 5 do Brisa: pesos do chão por amostra (a mesma conta de _vertice do
	# Ventania: areia, rocha, terra), costa e faixas de pedestres.
	var pesos := []
	for j in 201:
		for i in 201:
			var w := _pesos(m, i, j)
			pesos.append(snappedf(w.x, 0.01))
			pesos.append(snappedf(w.y, 0.01))
			pesos.append(snappedf(w.z, 0.01))
	var costa := []
	for k in 360:
		costa.append(snappedf(m.coast_radius(k * TAU / 360.0), 0.01))
	var faixas := []
	for f in m._faixas:
		faixas.append([snappedf(f[0].x, 0.01), snappedf(f[0].y, 0.01), snappedf(f[1].x, 0.001), snappedf(f[1].y, 0.001), f[2]])
	var d := {"fonte": "ventania game/world/ilha/ilha_ventania.gd", "pesos": pesos, "costa": costa, "faixas": faixas,
		"nivel_mar": m.NIVEL_MAR, "semente": m.SEMENTE, "relevo": {"n": 201, "passo": 3.0, "origem": -300.0, "alturas": alt},
		"pecas": pecas, "props": props, "baus": baus, "loot": loot, "pois": pois, "estradas": estradas, "escadas": esc, "spawns": spawns, "mobilia_n": mob.size(), "mobilia_amostra": mob.slice(0, 3)}
	var f := FileAccess.open(out_path, FileAccess.WRITE)
	f.store_string(JSON.stringify(d))
	f.close()
	print("EXTRAIDO pecas=%d props=%d baus=%d loot=%d pois=%d estradas=%d escadas=%d spawns=%d mobilia=%d" % [pecas.size(), props.size(), baus.size(), loot.size(), pois.size(), estradas.size(), esc.size(), spawns.size(), mob.size()])
	quit()


## Cópia da conta de ilha_ventania.gd::_vertice (só os pesos; não altera nada).
func _pesos(m, i: int, j: int) -> Vector3:
	var h: float = m._amostra(i, j)
	var x: float = -m.LIMITE + i * m.PASSO
	var z: float = -m.LIMITE + j * m.PASSO
	var t: float = m.inland(x, z)
	var inclinacao: float = absf(m._amostra(i + 1, j) - m._amostra(i - 1, j)) + absf(m._amostra(i, j + 1) - m._amostra(i, j - 1))
	var areia := clampf(1.0 - (t - 5.0) / 10.0, 0.0, 1.0) * clampf(1.0 - (h - 1.0) / 2.5, 0.0, 1.0)
	var rocha := clampf((inclinacao - 3.4) / 2.5, 0.0, 0.85) + clampf((h - 22.0) / 6.0, 0.0, 0.7)
	var terra := 0.0
	var dd: float = m._dist_estrada[j * m.N + i]
	if m._tipo_estrada[j * m.N + i] == 1:
		terra = clampf(1.0 - (dd + 0.5) / 3.0, 0.0, 1.0)
	else:
		terra = clampf(1.0 - (dd + 0.5) / 2.0, 0.0, 0.45)
	for a in m._lagos_terra:
		var dl := Vector2(x, z).distance_to(a[0])
		if dl < float(a[1]):
			if a.size() > 2:
				rocha = maxf(rocha, clampf((float(a[1]) - dl) / 8.0, 0.0, float(a[2])))
			else:
				terra = maxf(terra, clampf((float(a[1]) - dl) / 6.0, 0.0, 0.9))
	return Vector3(areia, minf(rocha, 1.0 - areia), terra)

