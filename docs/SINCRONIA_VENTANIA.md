# Sincronia com o Ventania

O Ventania continua sendo desenvolvido. O Brisa o consulta como referência, mas
**não segue cada mudança automaticamente**.

## Versão de referência

| Data | Commit do Ventania | Versão | Uso no Brisa |
|---|---|---|---|
| 2026-11-18 | `7864cd7` | v1.2.1 | Estudo da Etapa 0, auditoria dos 90 GLB, extração da Ilha |

## Como sincronizar (a cada atualização relevante do Ventania)

1. Clonar ou atualizar o Ventania **numa pasta separada** e registrar o commit novo aqui (nunca editar o Ventania).
2. Rodar a auditoria e o extrator e comparar com a versão anterior:
   ```bash
   python3 tools/auditoria/auditar_ventania.py --ventania ../ventania --json /tmp/novo.json
   ```
   ```bash
   GODOT=/caminho/godot tools/auditoria/extrair_ilha.sh ../ventania /tmp/ilha_nova.json
   ```
3. Ler `git log <commit-anterior>..<novo>` do Ventania e o ROADMAP de lá.
4. Para cada mudança, decidir e registrar na tabela abaixo:
   - **adotar** (vale igual no PS2);
   - **adaptar** (vale com simplificação);
   - **ignorar** (não cabe no PS2, ou é de rede/online);
   - **adiar**.
5. Mudança de **valores** (armas, Tormenta, passe) entra pelos `data/*.json` do Brisa, sem ELF novo. Mudança de **sistema** vira tarefa numa etapa do roadmap.

## Decisões registradas

| Data | Mudança no Ventania | Decisão | Motivo |
|---|---|---|---|
| 2026-11-18 | (base) v1.2.1 inteira | ver `docs/estudo/SISTEMAS_VENTANIA.md` | Etapa 0 |
