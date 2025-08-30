from POO.matriculados import Matriculados
from POO.professor import Professor
from POO.aluno import Aluno
from POO.monitor import Monitor


class Registro():

  def __init__(self):
    """Inicializa o registro com uma lista vazia para armazenar os dados."""
    self.__todosDados = []

  def inserir(self, pessoa):
    """
    Adiciona um novo registro à lista.
    Verifica se o objeto é uma instância de Matriculados antes de inserir.
    """
    if isinstance(pessoa, Matriculados):
      self.__todosDados.append(pessoa)
      print(f"'{pessoa.getNome()}' inserido(a) com sucesso.")
      return True
    else:
      print(
          "Erro: O objeto a ser inserido não é um tipo válido de Matriculado.")
      return False

  def remover(self, matricula):
    """
    Remove um registro da lista com base no número de matrícula.
    """
    pessoa_encontrada = None
    for pessoa in self.__todosDados:
      if pessoa.getMatricula() == matricula:
        pessoa_encontrada = pessoa
        break

    if pessoa_encontrada:
      self.__todosDados.remove(pessoa_encontrada)
      print(
          f"Registro com matrícula {matricula} ('{pessoa_encontrada.getNome()}') foi removido."
      )
      return True
    else:
      print(f"Erro: Matrícula {matricula} não encontrada.")
      return False

  def listarDados(self):
    """
    Imprime os dados de todos os registros na lista.
    Demonstra o polimorfismo em ação.
    """
    print("\n--- LISTA DE REGISTROS ---")
    if not self.__todosDados:
      print("Nenhum registro encontrado.")
      return

    for pessoa in self.__todosDados:
      pessoa.imprime()
    print("--- FIM DA LISTA ---\n")

  def ordenarDados(self, chave):
    """Ordena a lista de dados por nome ou matrícula."""
    if chave == 'nome':
      # Ordena a lista usando o nome
      self.__todosDados.sort(key=lambda pessoa: pessoa.getNome())
      print("Dados ordenados por nome.")
    elif chave == 'matricula':
      # Ordena a lista usando a matrícula
      self.__todosDados.sort(key=lambda pessoa: pessoa.getMatricula())
      print("Dados ordenados por matrícula.")
    else:
      print("Chave de ordenação inválida.")

  def listarReprovados(self):
    """Lista todos os alunos reprovados."""
    print("\n--- ALUNOS REPROVADOS ---")
    encontrou = False
    for pessoa in self.__todosDados:
      if isinstance(pessoa, Aluno):
        # Média abaixo de 5
        reprovado_por_media = pessoa.calcularMedia() < 5
        # Faltas acima de 25% (25% de 30 aulas = 7.5 faltas)
        reprovado_por_falta = pessoa.getFaltas() > 7.5

        if reprovado_por_media or reprovado_por_falta:
          pessoa.imprime()
          encontrou = True

    if not encontrou:
      print("Nenhum aluno reprovado encontrado.")
    print("--- FIM DA LISTA DE REPROVADOS ---\n")

  def listarAprovados(self):
    """Lista todos os alunos aprovados."""
    print("\n--- ALUNOS APROVADOS ---")
    encontrou = False
    for pessoa in self.__todosDados:
      if isinstance(pessoa, Aluno):
        if pessoa.calcularMedia() >= 5 and pessoa.getFaltas() <= 7.5:
          pessoa.imprime()
          encontrou = True

    if not encontrou:
      print("Nenhum aluno aprovado encontrado.")
    print("--- FIM DA LISTA DE APROVADOS ---\n")

  def get_alunos_aprovados(self):
    """Retorna uma lista de alunos aprovados."""
    return [
        aluno for aluno in self.__todosDados
        if isinstance(aluno, Aluno) and aluno.isAprovado()
    ]

  def get_alunos_reprovados(self):
    """Retorna uma lista de alunos reprovados."""
    return [
        aluno for aluno in self.__todosDados
        if isinstance(aluno, Aluno) and not aluno.isAprovado()
    ]
