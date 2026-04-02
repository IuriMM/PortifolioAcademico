from POO.matriculados import Matriculados


class Aluno(Matriculados):
  """attributes:
  __curso
  """

  def __init__(self, nome, contaativa, curso, faltas):
    super().__init__()
    self.setContaAtiva(contaativa)
    self.setNome(nome)
    self.__curso = curso
    self.__faltas = faltas
    self.__notas = []

  def getNotas(self):
    """Retorna a lista de notas do aluno."""
    return self.__notas

  def getTipoEntidade(self):
    return "Aluno"

  def getCurso(self):
    return self.__curso

  def setCurso(self, novocurso):
    self.__curso = novocurso
    return self.__curso

  def getFaltas(self):
    return self.__faltas

  def setFaltas(self, qtde):
    self.__faltas = qtde
    return self.__faltas

  def adicionarNota(self, nota):
    if len(self.__notas) < 4:
      self.__notas.append(float(nota))
      print(f"Nota {nota} adicionada com sucesso.")
    else:
      print("Erro: O aluno já possui 4 notas. Não é possível adicionar mais.")

  def calcularMedia(self):
    if not self.__notas:
      return 0.0

    media = sum(self.__notas) / len(self.__notas)
    return media

  def isAprovado(
      self,
      media_minima=5.0,
      faltas_maximas=30):  # Supondo 25% de faltas em 100 aulas totais
    """Verifica se o aluno foi aprovado."""
    if self.__faltas > faltas_maximas / 2 and self.__faltas < faltas_maximas:
      return self.calcularMedia() >= media_minima + 2
    return self.calcularMedia(
    ) >= media_minima and self.__faltas <= faltas_maximas

  def imprime(self):
    super().imprime()
    print("Curso: ", self.__curso)
    print("Notas: ", self.__notas)
    print(f"Média Final: {self.calcularMedia():.2f}")  #
    print("Quant. de faltas: ", self.__faltas)
