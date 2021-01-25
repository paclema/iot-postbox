import { Component, OnInit } from '@angular/core';
import { ConfigService } from '../config.service';
import { EnrollmentService } from '../enrollment.service';
import { PostConfigTabsService } from '../post-config-tabs.service';

// For the (manual) Reactive Form:
// import { FormGroup, FormControl } from '@angular/forms';
// For the (auto) Reactive Form:
import { FormBuilder, Validators, FormGroup, FormArray }  from '@angular/forms';
import { ForbiddenNameValidator }  from './shared/user-name.validator';
import { PasswordValidator }  from './shared/password.validator';

// Nebular:
import { NbCardModule } from '@nebular/theme';


@Component({
  selector: 'app-config-tabs',
  templateUrl: './config-tabs.component.html',
  styleUrls: ['./config-tabs.component.scss']
})
export class ConfigTabsComponent implements OnInit {

  public configData;
  public errorMsg;


  // For the Template Driven Form:
  public errorMsgPost = false;
  public dataMsgPost;

  public enableTutorial = false;

  testFormModel = {
    "userName": "Pablo",
    "email": "email@gmail.com",
    "phone": "666666666",
    "sleepMode": "Keep awake",
    "timePreference": "evening",
    "ota": "true",

  };

  sleepModes = ['Deep sleep', 'Light sleep', 'Keep awake'];
  sleepModeHasError = false;

  submitted = false;

  // For the (manual) Reactive Form:
  // registrationForm = new FormGroup({
  //   userName: new FormControl('paclema'),
  //   password: new FormControl(''),
  //   confirmPassword: new FormControl(''),
  //   address: new FormGroup({
  //     city: new FormControl('Coslada'),
  //     state: new FormControl('Madrid'),
  //     postalCode: new FormControl('28820')
  //   })
  // });

  // For the (auto) Reactive Form:
  // Moved to OnInit() to add conditional validation and subscribe for the observable
  /*
  registrationForm = this.fb.group({
    userName: ['', [Validators.required, Validators.minLength(3), ForbiddenNameValidator(/admin/)]],
    email: [''],
    subscribe: [false],
    password: [''],
    confirmPassword: [''],
    address: this.fb.group({
      city: [''],
      state: [''],
      postalCode: ['']
    })
  }, {validator: PasswordValidator});
  */
  // And add the FormGroup property:
  registrationForm: FormGroup;
  configTabsForm: FormGroup;


  constructor(private _configService: ConfigService,
              private _enrollmentService: EnrollmentService,
              private fb: FormBuilder,
              private _postConfigTabsService: PostConfigTabsService,
            ) {

    this.configTabsForm = this.fb.group({});
  }

  ngOnInit(): void {

    // Subscribe to the Observable received within the HTTP request to get the data
    this._configService.getConfigData()
          .subscribe(data => this.configData = data,
                      error => this.errorMsg = error);

    // console.log(this.configData);
    // this.loadApiData();

    // For the (auto) Reactive Form:
    this.registrationForm = this.fb.group({
      userName: ['', [Validators.required, Validators.minLength(3), ForbiddenNameValidator(/admin/)]],
      email: [''],
      subscribe: [false],
      password: [''],
      confirmPassword: [''],
      address: this.fb.group({
        city: [''],
        state: [''],
        postalCode: ['']
      }),
      alternateEmails: this.fb.array([])    // Dynamic FormArray initialy empty
    }, {validator: PasswordValidator});

    // Subscribe to the Observable to receive "subscribe" FormControl value changes:
    this.registrationForm.get('subscribe').valueChanges
          .subscribe(checkedValue => {
            const email = this.registrationForm.get('email');
            if (checkedValue){
              email.setValidators(Validators.required);
            } else {
              email.clearValidators();
            }
            email.updateValueAndValidity();
          });


    this.loadConfigFile();


  }

  // For the Template Driven Form:
  validateSleepMode(value){
    if((value === "default" ) || (value === ''))
      this.sleepModeHasError = true;
    else
      this.sleepModeHasError = false;
  }

  onSubmit(testForm){

    // The property of this class:
    console.log(this.testFormModel);
    // this.errorMsgPost = false;

    // The whole ngFormGroup status and data, received onSubmit():
    console.log(testForm);

    this._enrollmentService.enroll(this.testFormModel)
      .subscribe(
        data => {console.log('Success posting the data', data);
                  this.dataMsgPost = data;
                  this.submitted = true;},
        error => {console.log('Error posting the data', error);
                  this.errorMsgPost = error;
                  this.submitted = false;}
      )

  }


  // For the Reactive Form:
  loadApiData(){
    // To set each FormControl values for the Reactive FormGrop:
    // setValue() accepts an object that matches the structure of the FormGroup
    // the object must contain all keys

    // this.registrationForm.setValue({
    //   userName: 'name',
    //   password: 'password',
    //   confirmPassword: 'password',
    //   address: {
    //     city: 'City',
    //     state:'State',
    //     postalCode: '123456'
    //     }
    //   });

    // If only a few keys want to be set use patchValue():
    this.registrationForm.patchValue({
      userName: 'name',
      // password: 'password',
      // confirmPassword: 'password',
      address: {
        city: 'City',
        state:'State',
        // postalCode: '123456'
        }
      });
  }

  loadConfigFile(){
    this._configService.getConfigData()
          .subscribe(data => this.buildTabsForm(data),
                      error => this.errorMsg = error);
  }

  get userName(){
    return this.registrationForm.get('userName');
  }

  get email(){
    return this.registrationForm.get('email');
  }

  get alternateEmails(){
    return this.registrationForm.get('alternateEmails') as FormArray;
  }

  addAlternativeEmails(){
    this.alternateEmails.push(this.fb.control(''));
  }

  onSubmitRF(){
    console.log(this.registrationForm.value);

    this._postConfigTabsService.register(this.registrationForm.value)
    .subscribe(
      response => {console.log('Success posting the data', response);
                this.dataMsgPost = response;
                this.submitted = true;},
      error => {console.log('Error posting the data', error);
                this.errorMsgPost = error;
                this.submitted = false;}
    )

  }

  saveConfigTabs(){
    // console.log(this.configTabsForm.value);

    this._postConfigTabsService.saveConfig(this.configTabsForm.value)
    .subscribe(
      response => {console.log('Success posting the data', response);
                this.dataMsgPost = response;
                this.submitted = true;},
      error => {console.log('Error posting the data', error);
                this.errorMsgPost = error;
                this.submitted = false;}
    )

  }

  restartDevice(){
    // console.log(this.configTabsForm.value);

    this._postConfigTabsService.restartDevice()
    .subscribe(
      response => {console.log('Success restarting the device', response);
                this.dataMsgPost = response;
                this.submitted = true;},
      error => {console.log('Error restarting the device', error);
                this.errorMsgPost = error;
                this.submitted = false;}
    )

  }

  gpioTest(){
    // console.log(this.configTabsForm.value);
    const id = "LED_BUILTIN";
    const val = true;

    this._postConfigTabsService.gpioTest(id, val)
    .subscribe(
      response => {console.log('Success testing GPIO', response);
                this.dataMsgPost = response;
                this.submitted = true;},
      error => {console.log('Error testing GPIO', error);
                this.errorMsgPost = error;
                this.submitted = false;}
    )

  }

  configFactoryDefaults(){
    console.log("Restoring the backup for config.json...");
    // The property of this class:
    // console.log(this.testFormModel);
    // this.errorMsgPost = false;

    this._postConfigTabsService.restoreBackup("/config/config.json")
    .subscribe(
      response => {console.log('Success restoring config.json', response);
                this.dataMsgPost = response;
                this.submitted = true;},
      error => {console.log('Error restoring config.json', error);
                this.errorMsgPost = error;
                this.submitted = false;}
    )

  }

  buildTabsForm(configTabs){
    console.log('Building configTabsForm...');
    // console.log(configTabs);
    this.configTabsForm = this.fb.group({});

    for(let tab in configTabs) {
      let newTabForm = this.fb.group({});
      for(let ind in configTabs[tab]) {
        // newTabForm.addControl(ind, this.fb.control(configTabs[tab][ind]));
        // if(this.isArray(configTabs[tab][ind]))
        // console.log('ind:'+ ind + ' -> ' + configTabs[tab][ind] + ' -> '+ this.isObject(configTabs[tab][ind]));

        // If the value is another nested object, create another FormGroup with FormControls for each key:
        if(this.isObject(configTabs[tab][ind])){
          // console.log('ind:'+ ind + ' -> ' + configTabs[tab][ind] + ' is an obejct-> '+ this.isObject(configTabs[tab][ind]));
          let nestedConfigForm = this.fb.group({});
          for(let ind2 in configTabs[tab][ind]) {
            // Create nested FormControls with validator:
            nestedConfigForm.addControl(ind2, this.fb.control(configTabs[tab][ind][ind2], Validators.required));
          }
          // Add the nested FormGroup to the tab FormGroup:
          newTabForm.addControl(ind, nestedConfigForm);

        } else {
          newTabForm.addControl(ind, this.fb.control(configTabs[tab][ind], Validators.required));
        }

      }
      // console.log('newTabForm: ');
      // console.log(newTabForm);
      // newTabForm.setParent(this.configTabsForm);
      this.configTabsForm.addControl(tab, newTabForm);

    }
    console.log('Form built: configTabsForm');
    console.log(this.configTabsForm);
    // console.log('Form built: configTabsForm.value');
    console.log(this.configTabsForm.value);
  }

  // This function its added to use keyvalue pipe under *ngFor to get the
  // configTabsForm.controls unsorted:
  returnZero() {
    return 0
  }

  isNumber(val): boolean { return typeof val === 'number'; }
  isString(val): boolean { return typeof val === 'string'; }
  isBoolean(val): boolean { return typeof val === 'boolean'; }
  isArray(val): boolean {
    if (Array.isArray(val)) return true;
    return false; }
  isObject(val): boolean {
    if (Array.isArray(val)) return false
    return typeof val === 'object'; }
  isFile(key): boolean {
    if (key.includes("file")) {
      return true;
    } else
      return false;
  }

  typeConfig(obj): string {
    // console.log("Receiving obj");
    // console.log(obj);

    if (this.isNumber(obj)) {
      return 'number';
    } else if (this.isString(obj)) {
      return 'string';
    } else if (this.isBoolean(obj)) {
      return 'boolean';
    } else if (this.isArray(obj)) {
      return 'array';
    } else if (this.isObject(obj)) {
      return 'object';
    } else
      return null;

  }

  log(val) { console.log(val); }

  prettyPrint(text) {
    // var ugly = document.getElementById('myTextArea').value;
    // var obj = JSON.parse(text);
    var pretty = JSON.stringify(text.value, undefined, 4);
    // document.getElementById('myTextArea').value = pretty;
    return pretty;
  }

}