var Particle = require('particle-api-js');
var particle = new Particle();
var Phant = require('phant-client').Phant;
var phant = new Phant();

phant.connect(process.env.TEMPERATURE_PHANT_IRI, function(error, streamd) {
  if (error) {
    console.log(error);
  }

  streamd.privateKey = process.env.TEMPERATURE_PHANT_PRIVATE_KEY;

  var variables = {};
  var promises = [];

  for (var x = 0; x < 5; x++) {
    var variableName = "temp" + (x + 1);
    var promise = particle.getVariable({
      deviceId: process.env.TEMPERATURE_PARTICLE_DEVICE_ID,
      name: variableName,
      auth: process.env.TEMPERATURE_PARTICLE_ACCESS_KEY
    })
    .then(function(data) {
      variables[data.body.name] = data.body.result / 16;
    });
    promises.push(promise);
  }

  Promise.all(promises)
  .then(function() {
    phant.add(streamd, variables, function(error) {
      if (error) {
        console.log(error);
      }
      else {
        console.log("Done.");
      }
    });
  });
});
