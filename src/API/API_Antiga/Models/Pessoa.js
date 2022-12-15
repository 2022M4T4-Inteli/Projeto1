const { default: mongoose } = require('mongoose');

const Pessoa = mongoose.model('Pessoa', {
    
    RFID: String,
    distancia: Number
    
})

module.exports = Pessoa