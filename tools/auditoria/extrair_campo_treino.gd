extends SceneTree
# Reproduz o sorteio de CampoTreino._props() do Ventania (semente 42) e imprime JSON.
func _init() -> void:
	var rnd := RandomNumberGenerator.new()
	rnd.seed = 42
	var tipos := ["arvore_redonda_01", "pinheiro_01", "pinheiro_01", "pedra_01", "arbusto_01"]
	var out := []
	for i in 40:
		var ang := rnd.randf() * TAU
		var dist := rnd.randf_range(32.0, 110.0 - 8.0)
		var rot := rnd.randf() * 360.0
		out.append({"m": tipos[i % tipos.size()], "x": snappedf(cos(ang) * dist, 0.001), "z": snappedf(sin(ang) * dist, 0.001), "rot": snappedf(rot, 0.01)})
	print(JSON.stringify(out))
	quit()
