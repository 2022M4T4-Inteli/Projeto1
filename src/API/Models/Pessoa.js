const { default: mongoose } = require('mongoose');

// cria o schema do banco de dados
const Pessoa = mongoose.model('Pessoa', {
    
    RFID: String,
    distancia: Number,
    estado: Number,
    horario: String
    
})

// exporta o modelo do banco de dados criado
module.exports = Pessoa