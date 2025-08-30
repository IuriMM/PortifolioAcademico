# interface_grafica.py

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog

# Importando suas classes do backend
from POO.registro import Registro
from POO.aluno import Aluno
from POO.professor import Professor
from POO.monitor import Monitor


class AppGUI:

    def __init__(self, root):
        """Inicializa a aplicação da interface gráfica."""
        self.registro = Registro()
        self.root = root
        self.root.title("Sistema de Cadastro Acadêmico")
        self.root.geometry(
            "1150x650")  # ### ALTERADO: Aumentei a largura da janela

        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)

        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=5)

        self.btn_cadastrar = ttk.Button(button_frame,
                                        text="Cadastrar Nova Pessoa",
                                        command=self.abrir_janela_cadastro)
        self.btn_cadastrar.pack(side=tk.LEFT, padx=5)

        self.btn_remover = ttk.Button(button_frame,
                                      text="Remover por Matrícula",
                                      command=self.remover_pessoa)
        self.btn_remover.pack(side=tk.LEFT, padx=5)

        self.btn_aprovados = ttk.Button(button_frame,
                                        text="Listar Aprovados",
                                        command=self.listar_alunos_aprovados)
        self.btn_aprovados.pack(side=tk.LEFT, padx=5)

        self.btn_reprovados = ttk.Button(button_frame,
                                         text="Listar Reprovados",
                                         command=self.listar_alunos_reprovados)
        self.btn_reprovados.pack(side=tk.LEFT, padx=5)

        self.btn_atualizar = ttk.Button(button_frame,
                                        text="Atualizar Lista Geral",
                                        command=self.atualizar_lista)
        self.btn_atualizar.pack(side=tk.LEFT, padx=5)

        ### ALTERADO: Adicionada a coluna "Curso" na definição do Treeview ###
        self.tree = ttk.Treeview(main_frame,
                                 columns=("Matrícula", "Nome", "Função",
                                          "Curso", "Status", "Média",
                                          "Faltas"),
                                 show="headings")
        self.tree.heading("Matrícula", text="Matrícula")
        self.tree.column("Matrícula", width=80, anchor=tk.CENTER)
        self.tree.heading("Nome", text="Nome")
        self.tree.column("Nome", width=200)
        self.tree.heading("Função", text="Função")
        self.tree.column("Função", width=100, anchor=tk.CENTER)

        ### ALTERADO: Definição da nova coluna "Curso" ###
        self.tree.heading("Curso", text="Curso")
        self.tree.column("Curso", width=150)

        self.tree.heading("Status", text="Status")
        self.tree.column("Status", width=80, anchor=tk.CENTER)
        self.tree.heading("Média", text="Média")
        self.tree.column("Média", width=60, anchor=tk.CENTER)
        self.tree.heading("Faltas", text="Faltas")
        self.tree.column("Faltas", width=60, anchor=tk.CENTER)

        self.tree.pack(fill=tk.BOTH, expand=True, pady=10)

        self.atualizar_lista()

    def atualizar_lista(self):
        """Limpa a árvore e a preenche com os dados mais recentes do registro."""
        for item in self.tree.get_children():
            self.tree.delete(item)

        lista_pessoas = self.registro._Registro__todosDados

        for pessoa in lista_pessoas:

            try:
                matricula = pessoa.getMatricula(
                )  # Vamos tentar executar a linha problemática
                nome = pessoa.getNome()
                funcao = pessoa.getTipoEntidade()
                status = "Ativa" if pessoa.getContaAtiva() else "Inativa"

                curso = ""
                media = ""
                faltas = ""
                if isinstance(pessoa, Aluno):
                    curso = pessoa.getCurso()
                    media = f"{pessoa.calcularMedia():.2f}"
                    faltas = pessoa.getFaltas()

                self.tree.insert("",
                                 tk.END,
                                 values=(matricula, nome, funcao, curso,
                                         status, media, faltas))
            except AttributeError as e:
                print(f"!!!!!!!!!! ERRO ENCONTRADO AQUI: {e} !!!!!!!!!!")

    def remover_pessoa(self):
        matricula = simpledialog.askstring(
            "Remover Pessoa",
            "Digite a matrícula a ser removida:",
            parent=self.root)
        if matricula:
            if self.registro.remover(matricula):
                messagebox.showinfo(
                    "Sucesso",
                    f"Pessoa com matrícula {matricula} removida com sucesso!")
                self.atualizar_lista()
            else:
                messagebox.showerror("Erro",
                                     f"Matrícula {matricula} não encontrada.")

    def abrir_janela_cadastro(self):
        janela_cadastro = tk.Toplevel(self.root)
        janela_cadastro.title("Cadastrar Nova Pessoa")
        janela_cadastro.geometry("450x400")
        janela_cadastro.transient(self.root)
        janela_cadastro.grab_set()

        frame = ttk.Frame(janela_cadastro, padding="10")
        frame.pack(fill=tk.BOTH, expand=True)

        tipo_pessoa = tk.StringVar(value="aluno")

        def mostrar_campos():
            for widget in frame_campos.winfo_children():
                widget.destroy()

            tipo = tipo_pessoa.get()

            ttk.Label(frame_campos, text="Nome:").grid(row=0,
                                                       column=0,
                                                       sticky=tk.W,
                                                       pady=2)
            entry_nome = ttk.Entry(frame_campos, width=30)
            entry_nome.grid(row=0, column=1, sticky=tk.EW, pady=2)

            if tipo == "aluno":
                ttk.Label(frame_campos, text="Curso:").grid(row=1,
                                                            column=0,
                                                            sticky=tk.W,
                                                            pady=2)
                entry_curso = ttk.Entry(frame_campos)
                entry_curso.grid(row=1, column=1, sticky=tk.EW, pady=2)

                ttk.Label(frame_campos, text="Faltas:").grid(row=2,
                                                             column=0,
                                                             sticky=tk.W,
                                                             pady=2)
                entry_faltas = ttk.Entry(frame_campos)
                entry_faltas.grid(row=2, column=1, sticky=tk.EW, pady=2)

                ttk.Label(frame_campos,
                          text="Notas (N1,N2,N3,N4):").grid(row=3,
                                                            column=0,
                                                            sticky=tk.W,
                                                            pady=2)
                entry_notas_frame = ttk.Frame(frame_campos)
                entry_notas_frame.grid(row=3, column=1, sticky=tk.EW, pady=2)

                nota_entries = []
                for i in range(4):
                    entry = ttk.Entry(entry_notas_frame, width=5)
                    entry.pack(side=tk.LEFT, padx=2)
                    nota_entries.append(entry)

                def salvar_aluno():
                    try:
                        nome = entry_nome.get().strip()
                        curso = entry_curso.get().strip()
                        faltas = int(entry_faltas.get())

                        if not nome or not curso:
                            raise ValueError("Nome e curso são obrigatórios.")
                        if not (0 <= faltas <= 100):
                            raise ValueError("Faltas devem ser entre 0 e 100.")

                        novo_aluno = Aluno(nome=nome,
                                           contaativa=True,
                                           curso=curso,
                                           faltas=faltas)

                        notas_validas = []
                        for entry_n in nota_entries:
                            nota_str = entry_n.get().strip()
                            if nota_str:
                                nota = float(nota_str)
                                if 0 <= nota <= 10: notas_validas.append(nota)
                                else:
                                    raise ValueError(
                                        f"Nota inválida: {nota}. Deve estar entre 0 e 10."
                                    )
                            else:
                                raise ValueError("Preencha todas as 4 notas.")

                        if len(notas_validas) != 4:
                            raise ValueError(
                                "São necessárias exatamente 4 notas.")

                        for nota in notas_validas:
                            novo_aluno.adicionarNota(nota)

                        self.registro.inserir(novo_aluno)
                        self.atualizar_lista()
                        janela_cadastro.destroy()
                        messagebox.showinfo("Sucesso",
                                            f"Aluno '{nome}' cadastrado!")
                    except ValueError as e:
                        messagebox.showerror("Erro de Entrada",
                                             f"Dados inválidos: {e}")
                    except Exception as e:
                        messagebox.showerror("Erro Inesperado",
                                             f"Ocorreu um erro: {e}")

                btn_salvar = ttk.Button(frame_campos,
                                        text="Salvar Aluno",
                                        command=salvar_aluno)
                btn_salvar.grid(row=4, columnspan=2, pady=10)

            elif tipo == "professor":
                ttk.Label(frame_campos, text="Salário:").grid(row=1,
                                                              column=0,
                                                              sticky=tk.W,
                                                              pady=2)
                entry_salario = ttk.Entry(frame_campos)
                entry_salario.grid(row=1, column=1, sticky=tk.EW, pady=2)

                ttk.Label(frame_campos,
                          text="Qtd. Matérias:").grid(row=2,
                                                      column=0,
                                                      sticky=tk.W,
                                                      pady=2)
                entry_materias = ttk.Entry(frame_campos)
                entry_materias.grid(row=2, column=1, sticky=tk.EW, pady=2)

                def salvar_professor():
                    try:
                        nome = entry_nome.get().strip()
                        salario = float(entry_salario.get())
                        materias = int(entry_materias.get())

                        if not nome: raise ValueError("Nome é obrigatório.")
                        if not (salario > 0):
                            raise ValueError(
                                "Salário deve ser maior que zero.")
                        if not (materias >= 0):
                            raise ValueError(
                                "Quantidade de matérias não pode ser negativa."
                            )

                        novo_prof = Professor(nome, True, salario, materias, 0)
                        self.registro.inserir(novo_prof)
                        self.atualizar_lista()
                        janela_cadastro.destroy()
                        messagebox.showinfo("Sucesso",
                                            f"Professor '{nome}' cadastrado!")
                    except ValueError as e:
                        messagebox.showerror("Erro de Entrada",
                                             f"Dados inválidos: {e}")

                btn_salvar = ttk.Button(frame_campos,
                                        text="Salvar Professor",
                                        command=salvar_professor)
                btn_salvar.grid(row=3, columnspan=2, pady=10)

            elif tipo == "monitor":
                ttk.Label(frame_campos,
                          text="Valor da Bolsa:").grid(row=1,
                                                       column=0,
                                                       sticky=tk.W,
                                                       pady=2)
                entry_bolsa = ttk.Entry(frame_campos)
                entry_bolsa.grid(row=1, column=1, sticky=tk.EW, pady=2)

                ttk.Label(frame_campos,
                          text="Carga Horária:").grid(row=2,
                                                      column=0,
                                                      sticky=tk.W,
                                                      pady=2)
                entry_carga = ttk.Entry(frame_campos)
                entry_carga.grid(row=2, column=1, sticky=tk.EW, pady=2)

                def salvar_monitor():
                    try:
                        nome = entry_nome.get().strip()
                        bolsa = float(entry_bolsa.get())
                        carga = int(entry_carga.get())

                        if not nome: raise ValueError("Nome é obrigatório.")
                        if not (bolsa > 0):
                            raise ValueError(
                                "O valor da bolsa deve ser maior que zero.")
                        if not (carga > 0):
                            raise ValueError(
                                "A carga horária deve ser maior que zero.")

                        novo_monitor = Monitor(nome=nome,
                                               contaAtiva=True,
                                               valorbolsa=bolsa,
                                               cargahoraria=carga)
                        self.registro.inserir(novo_monitor)
                        self.atualizar_lista()
                        janela_cadastro.destroy()
                        messagebox.showinfo("Sucesso",
                                            f"Monitor '{nome}' cadastrado!")
                    except ValueError as e:
                        messagebox.showerror("Erro de Entrada",
                                             f"Dados inválidos: {e}")

                btn_salvar = ttk.Button(frame_campos,
                                        text="Salvar Monitor",
                                        command=salvar_monitor)
                btn_salvar.grid(row=3, columnspan=2, pady=10)

        ttk.Label(frame, text="Tipo de Cadastro:").pack()
        ttk.Radiobutton(frame,
                        text="Aluno",
                        variable=tipo_pessoa,
                        value="aluno",
                        command=mostrar_campos).pack(anchor=tk.W)
        ttk.Radiobutton(frame,
                        text="Professor",
                        variable=tipo_pessoa,
                        value="professor",
                        command=mostrar_campos).pack(anchor=tk.W)
        ttk.Radiobutton(frame,
                        text="Monitor",
                        variable=tipo_pessoa,
                        value="monitor",
                        command=mostrar_campos).pack(anchor=tk.W)

        frame_campos = ttk.Frame(frame, padding="10")
        frame_campos.pack(fill=tk.BOTH, expand=True)

        mostrar_campos()

    def _abrir_janela_lista_alunos(self, titulo, lista_alunos_func):
        janela_lista = tk.Toplevel(self.root)
        janela_lista.title(titulo)
        janela_lista.geometry("700x400")
        janela_lista.transient(self.root)
        janela_lista.grab_set()

        frame = ttk.Frame(janela_lista, padding="10")
        frame.pack(fill=tk.BOTH, expand=True)

        tree_alunos = ttk.Treeview(frame,
                                   columns=("Matrícula", "Nome", "Média",
                                            "Faltas", "N1", "N2", "N3", "N4"),
                                   show="headings")
        tree_alunos.heading("Matrícula", text="Matrícula")
        tree_alunos.column("Matrícula", width=80, anchor=tk.CENTER)
        tree_alunos.heading("Nome", text="Nome")
        tree_alunos.column("Nome", width=150)
        tree_alunos.heading("Média", text="Média")
        tree_alunos.column("Média", width=70, anchor=tk.CENTER)
        tree_alunos.heading("Faltas", text="Faltas")
        tree_alunos.column("Faltas", width=70, anchor=tk.CENTER)
        tree_alunos.heading("N1", text="N1")
        tree_alunos.column("N1", width=50, anchor=tk.CENTER)
        tree_alunos.heading("N2", text="N2")
        tree_alunos.column("N2", width=50, anchor=tk.CENTER)
        tree_alunos.heading("N3", text="N3")
        tree_alunos.column("N3", width=50, anchor=tk.CENTER)
        tree_alunos.heading("N4", text="N4")
        tree_alunos.column("N4", width=50, anchor=tk.CENTER)
        tree_alunos.pack(fill=tk.BOTH, expand=True)

        alunos = lista_alunos_func()

        if not alunos:
            ttk.Label(
                frame,
                text=f"Nenhum aluno {titulo.lower().split(' ')[1]} encontrado."
            ).pack(pady=10)
        else:
            for aluno in alunos:
                notas = aluno.getNotas() + [
                    "" for _ in range(4 - len(aluno.getNotas()))
                ]
                tree_alunos.insert("",
                                   tk.END,
                                   values=(aluno.getMatricula(),
                                           aluno.getNome(),
                                           f"{aluno.calcularMedia():.2f}",
                                           aluno.getFaltas(), notas[0],
                                           notas[1], notas[2], notas[3]))

        btn_fechar = ttk.Button(frame,
                                text="Fechar",
                                command=janela_lista.destroy)
        btn_fechar.pack(pady=10)

    def listar_alunos_aprovados(self):
        self._abrir_janela_lista_alunos("Alunos Aprovados",
                                        self.registro.get_alunos_aprovados)

    def listar_alunos_reprovados(self):
        self._abrir_janela_lista_alunos("Alunos Reprovados",
                                        self.registro.get_alunos_reprovados)


def iniciar_interface_grafica():
    root = tk.Tk()
    app = AppGUI(root)
    root.mainloop()


if __name__ == "__main__":
    iniciar_interface_grafica()
