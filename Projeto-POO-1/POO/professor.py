from POO.matriculados import Matriculados

class Professor(Matriculados):
  def __init__(self, nome, contaativa, salario, qtde_materias, cargahoraria):
    super().__init__()
    self.setContaAtiva(contaativa)
    self.setNome(nome)
    self.__qtde_materias = qtde_materias
    self.__salario = salario

  def getTipoEntidade(self):
    return "Professor"

  def setQtdeMaterias(self, qtde):
    self.__qtde_materias = qtde
    return self.__qtde_materias
    
  def getQtdeMaterias(self):
    return self.__qtde_materias

  def setSalario(self, novosal):
    self.__salario = novosal
    return self.__salario

  def getSalario(self):
    return self.__salario

  def imprime(self):
    super().imprime()
    print("Quant. de Matérias: ", self.__qtde_materias)
    print("Salário: ", self.__salario)