import datetime
from abc import ABC, abstractmethod


class Matriculados(ABC):
  """attributes:
  __nome
  __matricula
  __contaAtiva
  """

  # Atributo de classe (Sujeito a mudanças)
  __numeroDeMatriculados = 0

  def __init__(self):
    self.__nome = ""
    self.__matricula = ""
    self.__contaAtiva = False
    self.__gerarMatricula()
    self.__tipoEntidade = ""

  @abstractmethod
  def getTipoEntidade(self) -> str:
    pass

  def setTipoEntidade(self, cargo):
    self.__tipoEntidade = cargo
    return self.__tipoEntidade

  def getNome(self):
    return self.__nome

  def setNome(self, nome):
    self.__nome = nome
    return self.__nome

  def getMatricula(self):
    return self.__matricula

  def setMatricula(self, matricula):  # Tratar exceções
    self.__matricula = matricula
    return self.__matricula

  def getContaAtiva(self):
    return self.__contaAtiva

  def setContaAtiva(self, contaAtiva):  # Tratar exceções
    self.__contaAtiva = contaAtiva
    return self.__contaAtiva

  # Método privado (Usado apenas pela classe Matriculados)
  def __gerarMatricula(self):
    # Ano atual com 4 dígitos
    data: str = datetime.datetime.now().strftime("%Y")
    Matriculados.__numeroDeMatriculados += 1
    id = Matriculados.__numeroDeMatriculados
    nova_matricula = data + f"{id:04d}"  # Ex.: 20250001 (ano + ID com 4 dígitos)
    self.__matricula = nova_matricula

  def imprime(self):
    print("Nome: ", self.__nome)
    print("Matricula: ", self.__matricula)
    print("Função: ", self.getTipoEntidade())
    if self.__contaAtiva:
      print("Conta Ativa: SIM")
    else:
      print("Conta Ativa: NÃO")
