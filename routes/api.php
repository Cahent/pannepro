<?php

use App\Http\Controllers\Api\DeviceStatusController;
use Illuminate\Support\Facades\Route;
Route::post('/v1/devices/status', [DeviceStatusController::class, 'store'])->middleware('throttle:60,1');
